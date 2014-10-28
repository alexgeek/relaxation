#include <pthread.h>
#include <stdio.h>

void *sum_neighbour(void *arg) {
  int* idptr = (int*)arg;
  int id = *(idptr);
  printf("Thread ID = %d\n", ++id);

  return NULL;
}

int main() {
  pthread_t thread;
  int id = 41;
  if(pthread_create(&thread, NULL, sum_neighbour, &id))
  {
    fprintf(stderr, "Could not create thread.");
    return 1;
  }

  if(pthread_join(thread, NULL))
  {
    fprintf(stderr, "Could not join threads.");
    return 2;
  }
  
  return 0;
}
