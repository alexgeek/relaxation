#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED 314195

float* current;   // read from this grid
float* next;      // write to this grid
float* precision; // next - current
int n;            // grid dimensions
float p;          // precision

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
      g[n*i+j] = (i == 0 || i == n -1 || j == 0 || j == n-1) && 
        rand() % 4 > 0  ? 1.0f : 0.0f;
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
  next[l.i*n + l.j] = (current[n * l.i-1 + l.j] + current[n * l.i-1 + l.j] + current[n * l.i + l.j-1] + current[n * l.i + l.j+1])/4.0f;
  // calculate precision and store next[i][j]
  precision[l.i*n + l.j] = next[l.i*n + l.j] - current[l.i*n + l.j];

  // don't need to return anything from thread
  return NULL;
}

int main() {
  clock_t t0;
  clock_t t1;

  p = 0.1f;
  n = 8;

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

  int relaxing = 10000; // no of iterations

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

    int finished = 0;
    for(int i = 1; i < n-1; i++)
      for(int j = 1; j < n-1; j++)
        if(precision[i*n+j] < p)
          finished++;

    if(finished == (n*n) - (4*n-4))
    {
      printf("Reached %f precision in %d iterations.", p, relaxing);
      relaxing = 0;
    }

    // copy next to current TODO just swap pointers instead of copying.
    memcpy(current, next, n*n*sizeof(float));
  }

  t1 = clock();

  // display results (final grid and precision matrix)
  printf("The grid:\n\n");
  print_grid(next, n);
  printf("The precision:\n\n");
  print_grid(precision, n);
  
  printf("n = %d\n", n);
  printf("%fs elapsed.\n", 1000*(double)(t1-t0)/CLOCKS_PER_SEC);

  // free malloc'd arrays
  free(next);
  free(current);
  free(precision);

  return 0;
}
