// This simple program demonstrates how to arrange for
// multiple arguments to be passed to a pthread, getting 
// around the restriction to one argument imposed by
// the pthread_create interface.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// typedef for a pack of arguments (in this case, three integers)
typedef struct arg_pack {
  int myarg1;
  int myarg2;
  int myarg3;
} arg_pack;

typedef arg_pack *argptr;

// the thread function
void *mythreadcode (void *args) {
  int a, b, c;

  // retrieve the arguments
  a = ((arg_pack *)args)->myarg1;
  b = ((arg_pack *)args)->myarg2;
  c = ((arg_pack *)args)->myarg3;

  // use them
  printf ("Hello from thread %d, my other arguments were %d and %d.\n", a, b, c);
  return NULL;
}


int main (int argc, char* argv[]) {
  pthread_t  *threads;
  arg_pack   *threadargs;
  int i; 
  int p = 8; // we will create 8 threads

  // Set up handles and argument packs for the worker threads
  threads     = (pthread_t *) malloc (p * sizeof(pthread_t));
  threadargs  = (arg_pack *)  malloc (p * sizeof(arg_pack));

  // Create values for the arguments we want to pass to threads
  for (i=0; i<p; i++) {
    threadargs[i].myarg1 = i;
    threadargs[i].myarg2 = i*10;
    threadargs[i].myarg3 = i*100;
  }

  // Create the threads
  for (i=0; i<p; i++) {
    pthread_create (&threads[i], PTHREAD_CREATE_JOINABLE, mythreadcode, (void *) &threadargs[i]);
  }

  // Wait for the threads to finish
  for (i=0; i<p; i++) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}