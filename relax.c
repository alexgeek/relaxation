#define _XOPEN_SOURCE 600
// http://pages.cs.wisc.edu/~travitch/pthreads_primer.html
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmpfile.h"
#include "grid.h"

typedef int bool;
#define true 1
#define false 0

/* thread data struct */
typedef struct {
  int from, to;
} boundary;

/* program vars */
float* current;   // read from this grid
float* next;      // write to this grid
int dimension = 10;       // grid dimensions
float precision = 0.01f;  // precision

/* sync vars */
pthread_barrier_t barrier;
bool again = false;   // if should run another iteration
int thread_count = 8;     // number of threads
pthread_t* threads;   // array of threads
boundary* bounds;     // thread data (rows to span)

int partition_size() {
  return dimension / thread_count;
}

void assign_threads()
{
  threads = malloc(thread_count*sizeof(pthread_t));
  bounds = malloc(thread_count*sizeof(boundary));

  printf("hello");

  int size = partition_size();

  int i = 0;
  while (i < thread_count)
  {
    bounds[i] = (boundary){.from=i, .to=i + size};
    if (i == thread_count - 1) bounds[i].to = dimension - 1;
    i++;
  }
}

void *relax_part(void *arg) {
  // get rows to process
  boundary b = *(boundary*)arg;

  // work loop
  do {
    bool complete = true;
    int row;
    for(row = b.from; row < b.to; row++)
    {
      int col = 1;
      while (col < dimension-1) { // row minus the edges
        // slightly odd code to get neighbours on 1D array
        int i = row * dimension + col;
        next[i] = (current[i-1] + current[i+1] +
          current[i-dimension] + current[i+dimension]);
        // the precision check
        float p = next[i] - current[i];
        // if any do not meet the precision
        if(!(p < precision)) complete = false;
        i++;
      }
    }

    again = false; // all agree its false then wait

    int rc = pthread_barrier_wait(&barrier);
    if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    // if we need to continue flag it
    if (complete) {
      again = true;
    }

    // hack to run on just thread 1
    if (b.from == 1) {
      // copy new data (next) to the current for next iter
      memcpy(current, next, dimension*dimension*sizeof(float));
    }

    rc = pthread_barrier_wait(&barrier);
    if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    // now if anyone set again before the barrier,
    // then we'll have to keep going
    // otherwise the loop finishes and we exit

  } while(again);

  pthread_exit(NULL);
}


int to_colour (float f) {
  return 255 - floor(f == 1.0 ? 255 : f * 256.0);
}

void write_img()
{
  int width = dimension;
  int height = dimension;
  int depth = 24;

  bmpfile_t *bmp;
  rgb_pixel_t pixel = {0, 0, 0, 0};

  if ((bmp = bmp_create(width, height, depth)) == NULL) {
    fprintf(stderr, "Could not create bitmap");
    return;
  }

  int i, j;
  for (i = 0; i < dimension; i++) {
    for (j = 0; j < dimension; j++) {
      float f = current[i*dimension+j];
      pixel.red = to_colour(f);
      pixel.green = to_colour(f);
      pixel.blue = to_colour(f);
      //printf("<%d, %d> = <%d, %d, %d>\n", i, j, pixel.red, pixel.green, pixel.blue);
      bmp_set_pixel(bmp, j, i, pixel);
    }
  }

  bmp_save(bmp, "array.bmp");
  bmp_destroy(bmp);
}

int main() {
  clock_t t0;
  clock_t t1;

  // TODO command line opts


  // set up grids
  current = allocate_grid(dimension);
  next = allocate_grid(dimension);
  init_grid(current, dimension);
  init_grid(next, dimension);

  // init sync vars
  if(pthread_barrier_init(&barrier, NULL, thread_count))
  {
    fprintf(stderr, "Could not create barrier.");
    exit(4);
  }

  t0 = clock();

  // assign threads
  assign_threads();

  // start threads
  int i = 0;
  while(i < thread_count)
  {
    if(pthread_create(&threads[i], NULL, relax_part, &bounds[i]))
    {
      fprintf(stderr, "Could not create thread.");
      return 1;
    }
    i++;
  }

  // join threads
  i = 0;
  while(i < thread_count)
  {
    if(pthread_join(threads[i], NULL))
    {
      fprintf(stderr, "Could not create thread.");
      return 1;
    }
    i++;
  }


  t1 = clock();

  // display results (final grid and precision matrix)
  printf("The grid:\n\n");
  print_grid(next, dimension);

  printf("n = %d\n", dimension);
  printf("%fs elapsed.\n", 1000*(double)(t1-t0)/CLOCKS_PER_SEC);

  write_img();

  // free malloc'd arrays
  free(next);
  free(current);

  return 0;
}
