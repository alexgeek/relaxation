#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "thread.h"

int main(int argc, char **argv) {
  int n = 10;      // dimension
  float p = 0.01f; // precision
  int t = 8;      // threads
  int v = 0;      // verbose
  int i = 0;      // bitmap
  int opt;
  while ((opt = getopt (argc, argv, "n:p:t:v:i")) != -1)
  {
    switch(opt) {
      case 'n':
        if(sscanf(optarg, "%i", &n) != 1)
          fprintf(stderr, "Invalid opt for dimension, using default (%i).\n", n);
        break;
      case 'p':
        if(sscanf(optarg, "%f", &p) != 1)
          fprintf(stderr, "Invalid opt for precision, using default (%f).\n", p);
        if(p < 0) {
          fprintf(stderr, "Precision cannot be negative.\n");
        }
        break;
      case 't':
        if(sscanf(optarg, "%i", &t) != 1)
          fprintf(stderr, "Invalid opt (%s) for threads, using default (%i).\n", optarg, t);
        break;
      case 'v':
        v = 1;
        break;
      case 'i':
        i = 1;
        break;
    }
  }
  if(v && t > n-2) {
    fprintf(stderr, "Too many threads, using maximum of %d.\n", n-2);
    t = (n-2);
  }
  // t must be at least 1
  t = t < 1 ? 1 : t;
  if(v) printf("n = %d, p = %f, t = %d\n", n, p, t);
  float* result = relax_grid(n, p, t);
  if(i) {
    // write to a bitmap for quick visual checks
    write_img(result, n);
  }
  if(v) {
    print_grid(result, n);
  }
  free(result);
  return 0;
}
