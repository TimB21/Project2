#include <stdlib.h>  // for srand
#include <pthread.h> // POSIX Thread Library
#include <stdio.h>   // Standard I/O commands
#include <stdint.h>  // Used to suppress a warning when casting from (void*) to int
#include <stdbool.h> // Used to detect when writers have finished
#include <semaphore.h> // Used to implement semaphores

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

// Initializes writer and reader semaphores
sem_t writerS; // Semaphore used to control access to writing from the buffer
sem_t readerS; // Semaphore used to control access to reading from the buffer 
int readCount = 0; // Counter to keep track of the number of active reader threads

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
			// Check if there is a thread still writing
			if(stillWriting == true){
				sem_wait(&writerS); // Increment the writer semaphore to indicate that the writer wants to enter the critical section

				// WRITEUNIT() - Writing the thread to the buffer
				buffer[0] = threadID; // Write the id of the thread to the buffer
				randomDurationPause(); // Simulates write delay
				buffer[1] = threadID; // write the id of the thread to the buffer
				randomDurationPause(); // Simulates write delay

				sem_post(&writerS); // Decrement the writer semaphore to indicate that it has exited the critical section
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
	// Checks if there are still active writing threads
	if(stillWriting){
		// Increment the reader semaphore to indicate that the reader has priority 
		sem_wait(&readerS);
		// Increment the number of active readers
		readCount++;
		// If this is the first reader, prevent the writer from accessing the resources
		if(readCount == 1){
			sem_wait(&writerS);
		}
		sem_post(&readerS); 

		// READUNIT() - Reading the values from the buffer
		// Read the first value from the index buffer
		int index1 = buffer[0];
		// Use a reandom pause to simualte delay
		randomDurationPause();
		// Read the second value from the index buffer
		int index2 = buffer[1];
		// Check for consistency between indexes
		if(index1 == index2){ 
			// Prints out consistent index
			printf("Consistent Buffer of index: %d\n", index1);
		}
		else {
			// If the indexes are not the same, print out inconsistent indexes
			printf("Inconsistent Buffer with indexes:\n");
			printf("Index 1: %d\n", index1);
			printf("Index 2: %d\n", index2);
		}
		randomDurationPause(); 

		// Increment the reader seamphore to indicate that the reader wants to enter the critical section
		sem_wait(&readerS);
		// Decrement the number of active readers
		readCount--;
		// If there are no more active readers, signal that writers can proceed
		if(readCount == 0){
			// Decrement the writer semaphore to signal to writer that it can proceed
			sem_post(&writerS);
		}
		// Decrement the reader semaphore to indicate that the reader exits the critical section
		sem_post(&readerS);
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

	// Initialize reader semaphore with an initial value of 1
	sem_init(&readerS, 0, 1); 
	// Initialize writer semaphore with an initial value of 1
	sem_init(&writerS, 0, 1); 
	
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
