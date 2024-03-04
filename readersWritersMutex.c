#include <stdlib.h>  // for srand
#include <pthread.h> // POSIX Thread Library
#include <stdio.h>   // Standard I/O commands
#include <stdint.h>  // Used to suppress a warning when casting from (void*) to int
#include <stdbool.h> // Used to detect when writers have finished
#include <semaphore.h> 

/**
 * Number of times each writer thread executes its critical section
 */
#define WRITE_ACTIONS 20

/**
 * Number of reader and writer threads. It is particularly interesting
 * to have several writers because it creates a chance for different
 * values to be written to the buffer (race condition). The purpose of 
 * having multiple readers is to assure that each can access the buffer 
 * at the same time.
 */
#define NUM_READERS 3
#define NUM_WRITERS 3

/**
 * Maximum random pause that threads might be subjected to
 */
#define MAX_PAUSE 20000

/**
 * Block the currently running thread for a small random
 * duration in order to simulate the blocks that are
 * likely to occur in any real application that is writing
 * and reading data to and from a buffer.
 */
void randomDurationPause() {
	// Generate random pause duration
	int duration = rand() % MAX_PAUSE;
	struct timespec time1, time2;
   	time1.tv_sec = 0;
   	time1.tv_nsec = duration;
	// Block the thread to encourage more interesting interleaving of threads
	nanosleep(&time1, &time2); 
}

/**
 * This simple buffer is designed to hold two copies of an executing
 * writer's identifier. Random pauses will be used to delay writing to 
 * and reading from the buffer, which will encourage it to contain
 * inconsistent data if proper synchronization is lacking.
 */
int buffer[2];

/**
 * Will be true as long as any writer thread is active. Access to this
 * variable does not need to be synchronized because it will only ever
 * be written to once.
 */
bool stillWriting = true;

pthread_mutex_t writerMutex;
pthread_mutex_t readerMutex;
int readCount = 0;

/**
 * Each writer thread should execute its critical section WRITE_ACTIONS
 * number of times. The critical section should write the threadID to
 * both indices of the buffer, but a random pause is needed to encourage
 * a race condition.
 *
 * @param id Gets interpreted as an int representing the thread ID.
 * @return NULL returned to satisfy requirements of POSIX Threads.
 */
void * writer(void * id) {
	// Unique integer ID is sent to each writer thread
	int threadID = (intptr_t) id; // intptr_t used to suppress a casting warning
	// Announce that the thread has started executing
	printf("W%d entered\n", threadID); 
	int i;

	for(i = 0; i < WRITE_ACTIONS; i++) { // Consume a set number of values 
			if(stillWriting == true){
				pthread_mutex_lock(&writerMutex); // Decrement empty slots count

				// WRITEUNIT()
				buffer[0] = threadID;
				randomDurationPause();
				buffer[1] = threadID; 
				
				randomDurationPause();

				pthread_mutex_unlock(&writerMutex); // Exit critical section
			}
	}	
	printf("W%d finished\n", threadID); 
	return NULL;
}

/**
 * Each reader thread should execute its critical section as long as there 
 * are active writer threads. The critical section should read the values from
 * both indices of the buffer, but there should be a random pause to 
 * encourage a race condition. In addition, each read action should
 * send output to the console indicating whether the values read were
 * valid or erroneous.
 *
 * @param id Gets interpreted as an int representing the thread ID.
 * @return NULL returned to satisfy requirements of POSIX Threads.
 */
void * reader(void * id) {
	// Unique integer ID is sent to each reader thread
	int threadID = (intptr_t) id; // intptr_r used to suppress a casting warning
	// Announce that the thread has started executing
	printf("R%d entered\n", threadID); 
	while(stillWriting){
		pthread_mutex_lock(&readerMutex); // Decrement filled slots count
		readCount++;
		if(readCount == 1){
			pthread_mutex_lock(&writerMutex);
		}
    	pthread_mutex_unlock(&readerMutex); // Enter critical section 

		// READUNIT()
		int index1 = buffer[0];
		randomDurationPause(); 
		int index2 = buffer[1];
		if(index1 == index2){ 
			printf("Consistent Buffer of index: %d\n", index1);
		}
		else {
			printf("Inconsistent Buffer with indexes:\n");
			printf("Index 1: %d\n", index1);
			printf("Index 2: %d\n", index2);
		}
		randomDurationPause();

		pthread_mutex_lock(&readerMutex);
		readCount--;
		if(readCount == 0){
			pthread_mutex_unlock(&writerMutex);
		}
		pthread_mutex_unlock(&readerMutex);
	}

	printf("R%d finished\n", threadID); 
	return NULL;
}

/**
 * main function initializes reader and writer threads. 
 *
 * @return Success returns 0, crash/failure returns -1.
 */
int main() {
	printf("Readers/Writers Program Launched\n");
	
	if(pthread_mutex_init(&readerMutex, NULL)){
		printf("Unable to initilaize a mutex\n");
		return -1;
	}
	if(pthread_mutex_init(&writerMutex, NULL)){
		printf("Unable to initilaize a mutex\n");
		return -1;
	}
	pthread_t readerThread[NUM_READERS], writerThread[NUM_WRITERS];
	int id;

	// Launch writers
	for(id = 1; id <= NUM_WRITERS; id++) {
		if(pthread_create(&writerThread[id - 1], NULL, &writer, (void*)(intptr_t) id)) {
			printf("Could not create thread\n");
			return -1;
		}
	}

	// Launch readers
	for(id = 1; id <= NUM_READERS; id++) {
		if(pthread_create(&readerThread[id - 1], NULL, &reader, (void*)(intptr_t) id)) {
			printf("Could not create thread\n");
			return -1;
		}
	}

	printf("Threads initialized\n");

	// Writers complete
	for(id = 1; id <= NUM_WRITERS; id++) {
		if(pthread_join(writerThread[id - 1], NULL)) {
			printf("Could not join thread\n");
			return -1;
		}
	}

	stillWriting = false; // Let readers know that writing is finished

	// Readers complete
	for(id = 1; id <= NUM_READERS; id++) {
		if(pthread_join(readerThread[id - 1], NULL)) {
			printf("Could not join thread\n");
			return -1;
		}
	}

	return 0; // Successful termination
}
