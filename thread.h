#ifndef __thread_h__
#define __thread_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "bmpfile.h"
#include "grid.h"

#define ERR_BARRIER       1
#define ERR_COND          2
#define ERR_MUTEX         3
#define ERR_PARAMS        4
#define ERR_THREAD_CREATE 5
#define ERR_MEM_COPY      6

typedef struct {
  pthread_barrier_t start_barrier;
  pthread_cond_t finished_cond;
  pthread_mutex_t finished_mutex;
  int threads;
  int completed;
  int relaxed;
  int dimension;
  float precision;
  float* current;
  float* next;
} global;

typedef struct {
  global* g;
  int from;
  int to;
} params;

global* create_globals(int dimension, float precision, int threads);
void free_globals(global* globals);
params* create_params(global* g, int from, int to);
void free_params(params* p);
int partition_size(int n, int num_threads);
float check_precision(float new, float old, float p);
float relax(float left, float right, float up, float down);
int sync_repeat(pthread_barrier_t* barr, global* g);
int sync_continue(pthread_cond_t* cond, pthread_mutex_t* mtx, int* completed,
  int* threads, int* relaxed, int thread_relaxed);
int relax_row(float* current, float* next, int row, int length, float precision);
void *relax_thread(void* arg);
void start_threads(pthread_t* threads, global* g);
void join_threads(pthread_t* threads, int thread_count);
int relax_grid(int dimension, float precision, int threads);
int to_colour (float f);
void write_img(global* g);

#endif
