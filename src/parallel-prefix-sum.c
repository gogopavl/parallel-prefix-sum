// Note that NITEMS, NTHREADS and SHOWDATA should
// be defined at compile time with -D options to gcc.
// They are the array length to use, number of threads to use
// and whether or not to printout array contents (which is
// useful for debugging, but not a good idea for large arrays).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>


sem_t thread_semaphores[NTHREADS]; // Global semaphores for each thread
pthread_barrier_t barr; // Global barrier for all threads

typedef struct arg_pack {
  int id;
  int start_index;
  int end_index;
  int *data;
} arg_pack;

typedef arg_pack *argptr;



// Print a helpful message followed by the contents of an array
// Controlled by the value of SHOWDATA, which should be defined
// at compile time. Useful for debugging.
void showdata (char *message,  int *data,  int n) {
  int i; 

  if (SHOWDATA) {
      printf (message);
      for (i=0; i<n; i++ ){
      printf (" %d", data[i]);
      }
      printf("\n");
    }
}

// Check that the contents of two integer arrays of the same length are equal
// and return a C-style boolean
int checkresult (int* correctresult,  int *data,  int n) {
  int i; 

  for (i=0; i<n; i++ ){
    if (data[i] != correctresult[i]) return 0;
  }
  return 1;
}

// Compute the prefix sum of an array **in place** sequentially
void sequentialprefixsum (int *data, int n) {
  int i;

  for (i=1; i<n; i++ ) {
    data[i] = data[i] + data[i-1];
  }
}

// Compute the prefix sum of an array with specified indeces **in place** sequentially
void threadprefixsum (int *data, int start_index, int end_index) {
  int i;

  for (i = start_index+1; i < end_index + 1; i++) {
    data[i] = data[i] + data[i-1];
  }
}

// Thread function
void *thread_function(void *args)
{
  int i, id, start_index, end_index, *data, n;

  // Retrieve the arguments
  id = ((arg_pack *)args)->id;
  start_index = ((arg_pack *)args)->start_index;
  end_index = ((arg_pack *)args)->end_index;
  data = ((arg_pack *)args)->data;

  threadprefixsum(data, start_index, end_index); // Phase 1 - local chunk prefix sum calculation

  pthread_barrier_wait(&barr); // All threads completed Phase 1

  if (id == 0){ // First thread does not read the previous thread's last value
    sem_post(&thread_semaphores[id]); // Unlock semaphore to start "chain" of Phase 2
  }
  else { // All other threads read their previous thread's final cell
    int prev_final_val;
    sem_wait(&thread_semaphores[id-1]); // Wait for previous thread to release lock
    prev_final_val = data[start_index-1]; // Retrieve previous thread's last value 
    data[end_index] += prev_final_val; // Add previous thread's final value to my final value
    sem_post(&thread_semaphores[id]); // Release my lock in order for the next thread to get my new final value
    
    for (i = start_index; i < end_index; i++){
      data[i] += prev_final_val; // Update all other cells
    } 
  }

  return NULL;
}

// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
void parallelprefixsum (int *data, int n) {
  int i;
  
  for(i = 0; i < NTHREADS; i++)
    sem_init(&thread_semaphores[i], 0, 0);

  pthread_barrier_init(&barr, NULL, NTHREADS); // Barrier used for the synchronization of threads after Phase 1
  
  pthread_t  *thread_array;
  arg_pack   *threadargs;

  // Set up handles and argument packs for the worker threads
  thread_array = (pthread_t *) malloc (NTHREADS * sizeof(pthread_t));
  threadargs = (arg_pack *)  malloc (NTHREADS * sizeof(arg_pack));

  int cells_per_thread = NITEMS / NTHREADS;
  int remainder_of_div = NITEMS % NTHREADS;

  // Create values for the arguments we want to pass to threads
  for (i = 0; i < NTHREADS; i++) {
    threadargs[i].id = i; // Thread id
    threadargs[i].start_index = i * cells_per_thread; // Starting index
    threadargs[i].end_index = (i+1) * cells_per_thread - 1; // Ending index
    threadargs[i].data = data;
    if (i == NTHREADS - 1){
      threadargs[i].end_index += remainder_of_div; // Last thread takes more
    }
  }
  
  // Create the threads
  for (i = 0; i < NTHREADS; i++) {
    pthread_create (&thread_array[i], PTHREAD_CREATE_JOINABLE, thread_function, (void *) &threadargs[i]);
  }

  // Wait for the threads to finish
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(thread_array[i], NULL);
  }
  
}


int main (int argc, char* argv[]) {

  int *arr1, *arr2, i;

  // Check that the compile time constants are sensible for this exercise
  if ((NITEMS>10000000) || (NTHREADS>32)) {
    printf ("So much data or so many threads may not be a good idea! .... exiting\n");
    exit(EXIT_FAILURE);
  }

  // Create two copies of some random data
  arr1 = (int *) malloc(NITEMS*sizeof(int));
  arr2 = (int *) malloc(NITEMS*sizeof(int));
  srand((int)time(NULL));
  for (i=0; i<NITEMS; i++) {
     arr1[i] = arr2[i] = rand()%5;
  }
  showdata ("initial data          : ", arr1, NITEMS);

  // Calculate prefix sum sequentially, to check against later
  sequentialprefixsum (arr1, NITEMS);
  showdata ("sequential prefix sum : ", arr1, NITEMS);

  // Calculate prefix sum in parallel on the other copy of the original data
  parallelprefixsum (arr2, NITEMS);
  showdata ("parallel prefix sum   : ", arr2, NITEMS);

  // Check that the sequential and parallel results match
  if (checkresult(arr1, arr2, NITEMS))  {
    printf("Well done, the sequential and parallel prefix sum arrays match.\n");
  } else {
    printf("Error: The sequential and parallel prefix sum arrays don't match.\n");
  }

  free(arr1); free(arr2);
  return 0;
}