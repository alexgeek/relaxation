#ifndef __grid_h__
#define __grid_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED 314195

float* allocate_grid(int n);
void init_grid(float* g, int n);
void init_to(float* g, int n, float x);
void print_grid(const float* g, int n);

#endif
