#include "grid.h"

/*
 * Array helpers
 */

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
  int i, j;
  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      // if on edge, flip a coin to see if it we mark it as 1 or 0
      g[n*i+j] = (i == 0 || i == n -1 || j == 0 || j == n-1)
        /* && rand() % 4 > 0 */ ? 1.0f : 0.0f;
    }
  }
}

// set each value of g of size n to x
void init_to(float* g, int n, float x)
{
  int i, j;
  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      g[n*i+j] = x;
    }
  }
}

// utility function to print array of size n
void print_grid(const float* g, int n) {
  int i, j;
  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      printf("%.4f ", g[n*i+j]);
    }
    printf("\n");
  }
}
