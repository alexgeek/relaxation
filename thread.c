#include "thread.h"

global* create_globals(int dimension, float precision, int threads)
{
  global* g = (global*)malloc(sizeof(global));
  if(pthread_barrier_init(&(g->start_barrier), NULL, threads))
  {
    fprintf(stderr, "Could not create barrier.\n");
    exit(ERR_BARRIER);
  }
  if(pthread_cond_init(&(g->finished_cond), NULL)) {
    fprintf(stderr, "Could not create condition variable.\n");
    exit(ERR_COND);
  }
  if(pthread_mutex_init(&(g->finished_mutex), NULL)) {
    fprintf(stderr, "Could not create mutex.");
    exit(ERR_MUTEX);
  }
  g->threads = threads;
  g->completed = 0;
  g->relaxed = 0;
  g->dimension = dimension;
  g->precision = precision;
  g->current = allocate_grid(dimension);
  g->next = allocate_grid(dimension);
  init_grid(g->current, dimension);
  init_grid(g->next, dimension);
  return g;
}

void free_globals(global* g) {
  free(g->next);
  free(g->current);
  pthread_barrier_destroy(&(g->start_barrier));
  pthread_cond_destroy(&(g->finished_cond));
  pthread_mutex_destroy(&(g->finished_mutex));
  free(g);
}

params* create_params(global* g, int from, int to) {
  params* p = (params*)malloc(sizeof(params));
  if(p == NULL) {
    fprintf(stderr, "Cannot create params.");
    exit(ERR_PARAMS);
  }
  p->g = g;
  p->to = to;
  p->from = from;
  return p;
}

void free_params(params* p) {
  free(p);
}

int partition_size(int n, int num_threads) {
  return (n-2) / num_threads;
}

// returns true is precision has been met
float check_precision(float new, float old, float p) {
  return fabs(new-old) < p;
}

int swap_grid(float* current, float* next, int n)
{
  if(memcpy(current, next, n*n*sizeof(float)) == NULL) {
    fprintf(stderr, "Cannot copy memory.\n");
    exit(ERR_MEM_COPY);
  }
  return 1;
}

int sync_repeat(pthread_barrier_t* barr, global* g)
{
  int rc = pthread_barrier_wait(barr);
  if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    fprintf(stderr, "Cannot wait on barrier. rc = %d.\n", rc);
    exit(ERR_BARRIER);
  } else if(rc == PTHREAD_BARRIER_SERIAL_THREAD)
  {
    printf("Serial task.\n");
    g->completed = 0;
    g->relaxed = 0;
    swap_grid(g->current, g->next, g->dimension);
  }
  rc = pthread_barrier_wait(barr);
  return 1;
}

int sync_continue(pthread_cond_t* cond, pthread_mutex_t* mtx, int* completed,
  int* threads, int* relaxed, int thread_relaxed) {
  pthread_mutex_lock(mtx);
  (*completed)++;
  if(thread_relaxed)
    (*relaxed)++;
  if(*completed == *threads)
    pthread_cond_broadcast(cond);
  while(*completed < *threads)
    pthread_cond_wait(cond, mtx);
  int flag = (*relaxed < *threads);
  pthread_mutex_unlock(mtx);
  return flag;
}

int relax_row(float* current, float* next, int row, int length, float precision)
{
  int flag = 1; // flag as relaxed by default
  int col;
  // loop through row ignoring edges
  for(col = 1; col < length-1; col++)
  {
    // 2d indices -> 1d index
    int i = row*length + col;
    // the relaxation operation
    next[i] = relax(current[i-1], current[i+1], current[i+length], current[i-length]);
    // if any do not meet precision, flag as not relaxed
    if (!check_precision(next[i], current[i], precision)) flag = 0;
  }
  // 1 if row is relaxed, 0 if any are not relaxed yet
  return flag;
}

float relax(float left, float right, float up, float down)
{
  return (left + right + up + down)/4.0f;
}

void* relax_thread(void* arg)
{
  params par = *(params*)arg;
  float* current = (par.g->current);
  float* next = (par.g->next);

  int again;
  do {
    int row, relaxed = 1;
    for(row = par.from; row < par.to; row++)
      // if row not relaxed then set flag relaxed false
      if(!relax_row(current, next, row, par.g->dimension, par.g->precision))
        relaxed = 0;
    again = sync_continue(&(par.g->finished_cond), &(par.g->finished_mutex),
      &(par.g->completed), &(par.g->threads), &(par.g->relaxed), relaxed);
    if(again) {
      sync_repeat(&(par.g->start_barrier), par.g);
    }
  } while(again);

  free_params((params*)arg);
  pthread_exit(NULL);
}

void start_threads(pthread_t* threads, global* g)
{
  int size = partition_size(g->dimension, g->threads);
  int t = 0;
  do {
    int from = t*size + 1;
    int to = t < (g->threads-1) ? from + size : g->dimension-1;
    params* thread_args = create_params(g, from, to);
    if(pthread_create(&threads[t], NULL, relax_thread, thread_args))
    {
      fprintf(stderr, "Could not create thread.");
      exit(ERR_THREAD_CREATE);
    }
  } while(++t < g->threads);
}

void join_threads(pthread_t* threads, int thread_count)
{
  int t;
  for (t = 0; t < thread_count; t++)
    pthread_join(threads[t], NULL);
}

int relax_grid(int dimension, float precision, int threads)
{
  global* g = create_globals(dimension, precision, threads);
  pthread_t* thread_ptrs = (pthread_t*)malloc(threads*sizeof(pthread_t));
  start_threads(thread_ptrs, g);
  join_threads(thread_ptrs, threads);
  write_img(g);
  free(thread_ptrs);
  free_globals(g);
  return 1;
}


/* image writing functions */

// maps [0,1] to [0,256] for 8 bit colour.
int to_colour (float f) {
  return 255 - floor(f == 1.0 ? 255 : f * 256.0);
}

// write grid to bitmap
void write_img(global* g)
{
  int width = g->dimension;
  int height = g->dimension;
  int depth = 24;

  bmpfile_t *bmp;
  rgb_pixel_t pixel = {0, 0, 0, 0};

  if ((bmp = bmp_create(width, height, depth)) == NULL) {
    fprintf(stderr, "Could not create bitmap.\n");
    return;
  }

  print_grid(g->next, g->dimension);

  int i, j;
  for (i = 0; i < g->dimension; i++) {
    for (j = 0; j < g->dimension; j++) {
      float f = g->next[i*(g->dimension) + j];
      pixel.red = to_colour(f);
      pixel.green = to_colour(f);
      pixel.blue = to_colour(f);
      bmp_set_pixel(bmp, j, i, pixel);
    }
  }

  bmp_save(bmp, "grid.bmp");
  bmp_destroy(bmp);
}
