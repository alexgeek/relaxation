#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmpfile.h"

#define SEED 314195
#define MAX_ITR 10000

/* program vars */
float* current;   // read from this grid
float* next;      // write to this grid
float* precision; // next - current
int n;            // grid dimensions
float p;          // precision

/* sync vars */
pthread_mutex_t precision_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_cond_t precision_cond = PTHREAD_COND_INITIALIZER;
int nThreads;     // number of threads
int relaxed = 0;  // number of threads that reached precision

// allocate an array (using 1d array for 2d data)
float* allocate_grid(int n) {
  float* grid_ptr = (float*)malloc(sizeof(float)*n*n);
  if(grid_ptr==NULL)
  {
    fprintf(stderr, "Could not allocate grid.");
    exit(3);
  }
  return grid_ptr;
}

// sets edge values to 1 and inner values to 0
void init_grid(float* g, int n) {
  srand(SEED);
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < n; j++) {
      // if on edge, flip a coin to see if it we mark it as 1 or 0
      g[n*i+j] = (i == 0 || i == n -1 || j == 0 || j == n-1)
        /* && rand() % 4 > 0 */ ? 1.0f : 0.0f;
    }
  }
}

// set each value of g of size n to x
void init_to(float* g, int n, float x)
{
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < n; j++) {
      g[n*i+j] = x;
    }
  }
}

// utility function to print array of size n
void print_grid(const float* g, int n) {
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < n; j++) {
      printf("%.4f ", g[n*i+j]);
    }
    printf("\n");
  }
}

typedef struct {
  int i, j;
} loc;

// parallel task
void *sum_neighbour(void *arg) {
  // get the args
  loc l = *((loc*)arg);

  // debug code to identify element
  // printf("Element <%d,%d>; \n", l.i, l.j);

  // do the op
  next[l.i*n + l.j] = (current[n * (l.i-1) + l.j] + current[n * (l.i+1) + l.j] + current[n * l.i + l.j-1] + current[n * l.i + l.j+1])/4.0f;
  // calculate precision and store next[i][j]
  precision[l.i*n + l.j] = next[l.i*n + l.j] - current[l.i*n + l.j];

  // don't need to return anything from thread
  return NULL;
}

typedef struct {
  int from, to;
} bounds;

void *relax_part(void *arg) {
  // get bounds of subarray
  bounds b = *(bounds*)arg;

  int i = b.from;
  while (i < b.to) {
    next[i] = (current[i-1] + current[i+1] +
      current[i-n] + current[i+n]);
    precision[i] = next[i] - current[i];
    i++;
  }
}



int to_colour (float f) {
  return 255 - floor(f == 1.0 ? 255 : f * 256.0);
}

void write_img()
{
  int width = n;
  int height = n;
  int depth = 24;

  bmpfile_t *bmp;
  rgb_pixel_t pixel = {0, 0, 0, 0};

  if ((bmp = bmp_create(width, height, depth)) == NULL) {
    fprintf(stderr, "Could not create bitmap");
    return;
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      float f = current[i*n+j];
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

  p = 0.001f;
  n = 64;

  int iterations;

  // set up grids
  current = allocate_grid(n);
  next = allocate_grid(n);
  precision = allocate_grid(n);
  init_grid(precision, n);
  init_grid(current, n);
  init_grid(next, n);

  printf("The initial grid:\n");
  print_grid(precision, n);

  // threads and their parameters
  pthread_t* threads = malloc(n*n*sizeof(pthread_t));
  loc* locations = malloc(n*n*sizeof(loc));

  int relaxing = MAX_ITR; // no of iterations

  t0 = clock();

  while(relaxing-- > 0) {

    // create threads for each of the inner elements
    for(int i = 1; i < n-1; i++)
    {
      for(int j = 1; j < n-1; j++)
      {
        locations[i*n + j] = (loc){.i = i, .j = j};
        if(pthread_create(&threads[i*n + j], NULL, sum_neighbour, &locations[i*n + j]))
        {
          fprintf(stderr, "Could not create thread.");
          return 1;
        }
      }
    }

    // wait for all threads to finish before continuing.
    for(int i = 1; i < n-1; i++)
    {
      for(int j = 1; j < n-1; j++)
      {
        if(pthread_join(threads[i*n+j], NULL))
        {
          fprintf(stderr, "Could not join threads.");
          return 2;
        }
      }
    }

    // count threads that have reached precision
    int finished = 0;
    for(int i = 1; i < n-1; i++)
      for(int j = 1; j < n-1; j++)
        if(precision[i*n+j] < p)
          finished++;

    // check all the elements not on the edge have reached precision
    if(finished == (n*n) - (4*n-4))
    {
      iterations = MAX_ITR - relaxing;
      relaxing = 0; // aborts loop
    }

    // copy next to current
    memcpy(current, next, n*n*sizeof(float));
  }

  t1 = clock();

  // display results (final grid and precision matrix)
  printf("The grid:\n\n");
  print_grid(next, n);
  printf("The precision:\n\n");
  print_grid(precision, n);

  printf("n = %d\n", n);
  printf("Reached %f precision in %d iterations.", p, iterations);
  printf("%fs elapsed.\n", 1000*(double)(t1-t0)/CLOCKS_PER_SEC);

  write_img();

  // free malloc'd arrays
  free(next);
  free(current);
  free(precision);

  return 0;
}
