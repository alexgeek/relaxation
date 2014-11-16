#define _XOPEN_SOURCE 600
// http://pages.cs.wisc.edu/~travitch/pthreads_primer.html
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmpfile.h"
#include "thread.h"

int main() {
  relax_grid(10, 0.001f, 8);
  return 0;
}
