/** 
 * ~~~~~~~~~~~~~~~~~~
 * ~~~ Discussion ~~~
 * ~~~~~~~~~~~~~~~~~~
 * 
 * 1. Approach
 * 
 * The method "parallelprefixsum" is responsible for the following three main things:
 * 
 *      - Distribution of array to processes via specified indeces
 *      - Packing of arguments to be used by threads
 *      - Creation of threads
 *
 * Afterwards, each thread executes the "threadfunction" method. This method has been
 * written in a way that provides a level of abstraction so that the algorithm outline
 * is obvious. That is, all calculations have been moved to other methods and are inv-
 * oked through the thread method. This way, the implementation is more reader-friend-
 * ly as it is very easy to follow.
 * 
 * 2. Synchronization
 * 
 * Threads are synchronized in the "threadfunction" method with the use of a barrier.
 * More specifically, we have to synchronize them in two points during execution. Rig-
 * ht before Phase 2 and before Phase 3. The reason for this is that all threads shou-
 * ld have computed their local prefix sum before thread 0 calculates final element v-
 * alues. Additionally, thread 0 has to have finished computing final values of the o-
 * ther threads before they can start updating their chunks.
 * 
 * 3. Execution and Correctness
 * 
 * The correctness of the parallel execution was ensured since the algorithm produces
 * the exact same output in all cases. An automated script was created to execute the
 * code for every number of threads (1 to 32) and for an increasing number of elemen-
 * ts from the number of threads to the maximum allowed value.
 * 
 * Moreover, the performance of the algorithm was calculated with the use of <time.h>
 * library. The performance can be printed after uncommenting the appropriate section
 * in "main" method. It seems that for all cases the serial implementation runs fast-
 * er than the parallel one. The reason for this is that thread creation can be quite
 * costly. Since our calculations are not that demanding we realise that creating mu-
 * ltiple threads becomes an overkill.
 * 
 * **/

// Note that NITEMS, NTHREADS and SHOWDATA should
// be defined at compile time with -D options to gcc.
// They are the array length to use, number of threads to use
// and whether or not to printout array contents (which is
// useful for debugging, but not a good idea for large arrays).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

const int CELLS_PER_THREAD = NITEMS / NTHREADS; // Number of cells per thread (except last thread)
const int REMAINDER_OF_DIV = NITEMS % NTHREADS; // Remainding elements (for last thread)

clock_t start, mid, stop;

pthread_barrier_t barr; // Global barrier for all threads

// Data structure for packing all arguments per thread together
typedef struct arg_pack {
  int id; // Thread id
  int *data; // Global array
  int start_index; // Thread chunk starting index
  int end_index; // Thread chunk ending index
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

// Compute final value of every thread except the first one
void finalelementprefix (int *data) {
  int i, curr_final_index, next_final_index;

  for (i = 0; i < NTHREADS - 1; i++) {
    curr_final_index = (i+1) * CELLS_PER_THREAD - 1; // Ending index of current thread's chunk
    next_final_index = curr_final_index + CELLS_PER_THREAD; // Ending index of next thread's chunk
    if(i == NTHREADS - 2){
      next_final_index += REMAINDER_OF_DIV; // Last thread's chunk is larger
    }
    data[next_final_index] += data[curr_final_index];
  }
}

// Compute final values of own chunk
void updatelocalvalues (int *data, int start_index, int end_index) {
  int i, prev_final_val;

  prev_final_val = data[start_index-1]; // Retrieve previous thread's last value

  for (i = start_index; i < end_index; i++){
    data[i] += prev_final_val; // Update all other cells
  }
}

// Thread function
void *threadfunction(void *args)
{
  int i, id, start_index, end_index, *data;

  // Unpack the arguments
  id = ((arg_pack *)args)->id;
  data = ((arg_pack *)args)->data;
  start_index = ((arg_pack *)args)->start_index;
  end_index = ((arg_pack *)args)->end_index;

  threadprefixsum(data, start_index, end_index); // Phase 1 - Local chunk prefix sum calculation

  pthread_barrier_wait(&barr); // All threads completed Phase 1

  if (id == 0){ // Phase 2 - Thread 0 computes the prefix sum of final elements
    finalelementprefix(data);
  }

  pthread_barrier_wait(&barr); // End of Phase 2 - barrier is reusable since Pthreads re-initialise it once all threads are synchronised

  if(id != 0){ // Phase 3 - All other threads read their previous thread's final cell and update their chunks (except last value)
    updatelocalvalues(data, start_index, end_index);
  }
}

// Function that initialises and invokes necessary components for the parallel computation of the prefix sum algorithm
void parallelprefixsum (int *data, int n) {
  int i;
  pthread_barrier_init(&barr, NULL, NTHREADS); // Barrier used for the synchronization of threads after Phase 1 and Phase 2
  
  // Setting up threads and their arguments
  pthread_t  *thread_array;
  arg_pack   *threadargs;
  thread_array = (pthread_t *) malloc (NTHREADS * sizeof(pthread_t));
  threadargs = (arg_pack *)  malloc (NTHREADS * sizeof(arg_pack));

  // Assign values to thread arguments
  for (i = 0; i < NTHREADS; i++) {
    threadargs[i].id = i; // Thread id
    threadargs[i].start_index = i * CELLS_PER_THREAD; // Starting index of chunk
    threadargs[i].end_index = (i+1) * CELLS_PER_THREAD - 1; // Ending index of chunk
    threadargs[i].data = data;
    if (i == NTHREADS - 1){
      threadargs[i].end_index += REMAINDER_OF_DIV; // Last thread takes the remainding elements in its chunk
    }
  }
  
  // Create the threads
  for (i = 0; i < NTHREADS; i++) {
    pthread_create (&thread_array[i], PTHREAD_CREATE_JOINABLE, threadfunction, (void *) &threadargs[i]);
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
  
  start = clock(); // Start for serial implementation
  
  // Calculate prefix sum sequentially, to check against later
  sequentialprefixsum (arr1, NITEMS);
  showdata ("sequential prefix sum : ", arr1, NITEMS);
  
  mid = clock(); // Mid point - end for serial and start for parallel
  
  // Calculate prefix sum in parallel on the other copy of the original data
  parallelprefixsum (arr2, NITEMS);
  showdata ("parallel prefix sum   : ", arr2, NITEMS);
  
  stop = clock(); // End for parallel implementation

  double serial = ((double) (mid - start)) / CLOCKS_PER_SEC;
  double parallel = ((double) (stop - mid)) / CLOCKS_PER_SEC;
  printf("Serial execution runtime =     %fs\n", serial);
  printf("Parallel execution runtime =   %fs\n", parallel);

  // Check that the sequential and parallel results match
  if (checkresult(arr1, arr2, NITEMS))  {
    printf("Well done, the sequential and parallel prefix sum arrays match.\n");
  } else {
    printf("Error: The sequential and parallel prefix sum arrays don't match.\n");
  }

  free(arr1); free(arr2);
  return 0;
}