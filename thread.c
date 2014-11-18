#include "thread.h"

// initialises all globals in a struct that each thread has a pointer to
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

// tidying up
void free_globals(global* g) {
  free(g->next);
  free(g->current);
  pthread_barrier_destroy(&(g->start_barrier));
  pthread_cond_destroy(&(g->finished_cond));
  pthread_mutex_destroy(&(g->finished_mutex));
  free(g);
}

// thread parameters. contains pointer to globals.
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

// tidying up
void free_params(params* p) {
  free(p);
}

// to distribute rows across threads, ignores rows 1 and n
int partition_size(int n, int num_threads) {
  return (n-2) / num_threads;
}

// returns true is precision has been met
float check_precision(float new, float old, float p) {
  return fabs(new-old) < p;
}

float relax(float left, float right, float up, float down)
{
  return (left + right + up + down)/4.0f;
}

// copy next into current
int swap_grid(float* current, float* next, int n)
{
  if(memcpy(current, next, n*n*sizeof(float)) == NULL) {
    fprintf(stderr, "Cannot copy memory.\n");
    exit(ERR_MEM_COPY);
  }
  return 1;
}

// called if precision is not met
// contains preprocessing for next loop
int sync_repeat(pthread_barrier_t* barr, global* g)
{
  // wait for everyone to get to this point
  int rc = pthread_barrier_wait(barr);
  if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    fprintf(stderr, "Cannot wait on barrier. rc = %d.\n", rc);
    exit(ERR_BARRIER);
  } else if(rc == PTHREAD_BARRIER_SERIAL_THREAD)
  {
    // one thread will reset sync vars
    g->completed = 0;
    g->relaxed = 0;
    // and copy the "next" grid into the "current" grid
    swap_grid(g->current, g->next, g->dimension);
  }
  // we're set up for the next loop, now we wait for everyone again
  rc = pthread_barrier_wait(barr);
  if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    fprintf(stderr, "Cannot wait on barrier. rc = %d.\n", rc);
    exit(ERR_BARRIER);
  }
  return 1;
}

// checking if we need to continue
int sync_continue(pthread_cond_t* cond, pthread_mutex_t* mtx, int* completed,
  int* threads, int* relaxed, int thread_relaxed) {
  pthread_mutex_lock(mtx);
  // communicate that loop has finished for this thread
  (*completed)++;
  // if we met precision, increment counter
  if(thread_relaxed)
    (*relaxed)++;
  // all done so wake all sleeping
  if(*completed == *threads)
    pthread_cond_broadcast(cond);
  // sleep until all completed (avoiding spurious wakeups)
  while(*completed < *threads)
    pthread_cond_wait(cond, mtx);
  pthread_mutex_unlock(mtx);
  // returns false is all threads are relaxed else true
  return (*relaxed < *threads);
}

// relaxes one row
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

// the worker thread
void* relax_thread(void* arg)
{
  params par = *(params*)arg;
  float* current = (par.g->current);
  float* next = (par.g->next);

  int again;
  do {
    int row, relaxed = 1; // assume we did relax our part
    for(row = par.from; row < par.to; row++)
      // if row not relaxed then set flag relaxed false
      if(!relax_row(current, next, row, par.g->dimension, par.g->precision))
        relaxed = 0;
    // check if we need to repeat
    again = sync_continue(&(par.g->finished_cond), &(par.g->finished_mutex),
      &(par.g->completed), &(par.g->threads), &(par.g->relaxed), relaxed);
    // if we need to repeat, cue preprocessing
    if(again) {
      sync_repeat(&(par.g->start_barrier), par.g);
    }
  } while(again);

  // params were malloc'd so tidy up here
  free_params((params*)arg);
  pthread_exit(NULL);
}

// delegates rows to process to threads, starting them immediately
void start_threads(pthread_t* threads, global* g)
{
  int size = partition_size(g->dimension, g->threads);
  int t = 0;
  do {
    // start at 1 to avoid edges
    int from = t*size + 1;
    // special case for last thread
    int to = t < (g->threads-1) ? from + size : g->dimension-1;
    // create the thread parameters (needs to be freed)
    params* thread_args = create_params(g, from, to);
    if(pthread_create(&threads[t], NULL, relax_thread, thread_args))
    {
      fprintf(stderr, "Could not create thread.");
      exit(ERR_THREAD_CREATE);
    }
  } while(++t < g->threads);
}

// blocks until all threads exit
void join_threads(pthread_t* threads, int thread_count)
{
  int t;
  for (t = 0; t < thread_count; t++)
    pthread_join(threads[t], NULL);
}

// kicks it all off
int relax_grid(int dimension, float precision, int threads)
{
  global* g = create_globals(dimension, precision, threads);
  pthread_t* thread_ptrs = (pthread_t*)malloc(threads*sizeof(pthread_t));
  if(thread_ptrs == NULL)
  {
    fprintf(stderr, "Could not allocate thread pointers.\n");
    exit(ERR_MALLOC);
  }
  start_threads(thread_ptrs, g);
  join_threads(thread_ptrs, threads);
  // write to a bitmap for quick visual checks
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

  // TODO add debugging preprocessor
  //print_grid(g->next, g->dimension);

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
