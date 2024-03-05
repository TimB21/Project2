#include <stdlib.h>  // for srand
#include <pthread.h> // POSIX Thread Library
#include <stdio.h>   // Standard I/O commands
#include <stdint.h>  // Used to suppress a warning when casting from (void*) to int
#include <stdbool.h> // Used to detect when writers have finished

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
	// Perform the number of writes defined by the number of time each process executes it critical section
	for(i = 0; i < WRITE_ACTIONS; i++) {
			// Set the first value of the buffer to the thread ID
			buffer[0] = threadID; 
			// Perform a random duration pause between buffer updatesto encourage a race condition
			randomDurationPause(); 
			// Set the second value of the buffer to the thread ID
			buffer[1] = threadID;
	} 
	// Wait occurs after write finishes 
	// Encourages race condition
	randomDurationPause(); 
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
	
	// If the writer thread is still active
	if(stillWriting == true){
		// Retrieve the first value in the buffer
		int index1 = buffer[0];
		// Pause for a random duration to encourage race condition
		randomDurationPause(); 
		// Retrieve the second value in the buffer
		int index2 = buffer[1];
		// If both indexes in the buffer are equal
		if(index1 == index2){ 
			// The buffer has consistent indexes and we print out this value
			printf("Consistent Buffer containing index: %d\n", index1);
		}
		else {
			// the buffer has two different values, therefore it is inconsistent
			printf("Inconsistent Buffer with indexes:\n");
			// Prints out the two indexes to show inconsistency
			printf("Index 1: %d\n", index1);
			printf("Index 2: %d\n", index2);
		}
	}
	// Wait occurs after read read finishes
	// Encourages race condition
	randomDurationPause(); 
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
