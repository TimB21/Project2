#include <stdlib.h> // for srand
#include <pthread.h> // POSIX Thread Library
#include <stdio.h>   // Standard I/O commands
#include <stdint.h>  // Used to suppress a warning when casting from (void*) to int
#include <semaphore.h> 

/**
 * Number of elements that can be produced
 * before consumption must occur in order to progress.
 */
#define BUFFER_SIZE 10
/**
 * Number of elements that each producer thread 
 * will produce.
 */
#define PRODUCTION_LIMIT 25
/**
 * Number of elements that each consumer thread
 * will consume.
 */
#define CONSUMPTION_LIMIT 35

/**
 * Number of producer and consumer threads.
 * The total number of results produced by all
 * producers needs to equal or exceed the total
 * number of results consumed by all consumers,
 * because otherwise correctly coded consumer
 * threads will block forever waiting for values
 * that are never produced.
 */
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 2

/**
 * Maximum random pause that threads might be subjected to
 */
#define MAX_PAUSE 20000

/**
 * Block the currently running thread for a small random
 * duration in order to simulate the blocks that are
 * likely to occur in any real application that is producing
 * and consuming data.
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
 * Used to track the value being "produced".
 * Note that this variable should only ever
 * be accessed by one thread at a time.
 */
int globalProductionCounter = 0;

sem_t empty;
sem_t writer;
sem_t semaphore;

/**
 * Because this is just a simple model for understanding
 * mutual exclusion, the produce command just produces a
 * sequence of increasing integers, rather than do something
 * actually useful. Unlike in the examples in the book, this
 * command should only be called inside the critical section
 * of a thread. This function also includes a thread ID to
 * output and track the way the program works.
 *
 * @param threadID Unique identifier for the producing thread.
 * @return The int value that is "produced" (before incrementing)
 */
int produce(int threadID) {
	randomDurationPause();
	printf("P%d: produce %d\n", threadID, globalProductionCounter);
	return globalProductionCounter++;
}

/**
 * A value is "consumed" by printing it to the console.
 * Once again, this action is simple, but allows for easy
 * monitoring of how the code functions. The ID of the
 * thread performing the operation is also sent and printed
 * for better clarity, but this has nothing to do with
 * the canonical producer/consumer problem.
 *
 * @param threadID Unique identifier for the consuming thread.
 * @param toConsume The int value being "consumed"
 */
void consume(int threadID, int toConsume) {
	printf("C%d: consumed %d\n", threadID, toConsume);
	randomDurationPause();
}

/**
 * buffer is a circular queue, whose "front" and "back" are
 * designated by insertAt and removeAt. The synchronization
 * mechanisms in your code must assure that elements in the
 * buffer are never overwritten before being consumed, or
 * consumed when they don't exist (which result in some leftover
 * value in the buffer being consumed).
 */
int buffer[BUFFER_SIZE];
int insertAt = 0;
int removeAt = 0;

/**
 * Adds a newly produced int to the buffer at the next
 * available slot. You do not need to change this command.
 * However, you do need to assure that it is never called
 * when the buffer is full.
 *
 * @param toAdd int value being added to the buffer.
 */
void append(int toAdd) {
    buffer[insertAt] = toAdd;
    insertAt = (insertAt + 1) % BUFFER_SIZE;
}

/**
 * Take the next available item in the circular queue buffer.
 * Value is returned, but not actually removed from the queue.
 * However, correct usage will assure that any value taken out
 * will be overwritten by a new value before a value is taken
 * from the same location again. In particular, this function
 * should never be called when the logical buffer (collection
 * of un-consumed elements) is empty. Do not change this function.
 *
 * @return int value that was removed from the buffer.
 */
int take() {
    int result = buffer[removeAt];
    removeAt = (removeAt + 1) % BUFFER_SIZE;
    return result;
}

/**
 * Producer thread keeps calling produce and appending the
 * result to the buffer. Proper synchronization must assure
 * mutually exclusive access to the buffer, and prevent
 * buffer overflow.
 * TODO: Add synchronization
 *
 * @param arg Gets interpreted as an int representing the thread ID.
 * @return NULL returned to satisfy requirements of POSIX Threads.
 */
void * producer(void * arg) {
	// Unique integer ID is sent to each producer thread
	int threadID = (intptr_t) arg; // intptr_t used to suppress a casting warning
	// Announce that the thread has started executing
	printf("P%d entered\n", threadID); 
	int i;
	for(i = 0; i < PRODUCTION_LIMIT; i++) { // Produce a set number of values
		sem_wait(&empty); // Decrement empty slots count
    	sem_wait(&semaphore); // Enter critical section
		// Start Critical Section: produce() must appear here to protect globalProductionCounter
		int producedResult = produce(threadID); // produce new value 
		append(producedResult); // add new value to buffer 
		sem_post(&semaphore); // Exit critical section
    	sem_post(&filled); // Increment filled slots count
		// End Critical Section

	}
	// Announce completion of thread 
	printf("P%d finished\n", threadID); 
	return NULL; // No value is returned by thread
}

/**
 * Consumer thread keeps taking a value from the buffer
 * and consuming it. Proper synchronization must assure
 * mutually exclusive access to the buffer, and prevent
 * taking of values from an empty buffer.
 * TODO: Add synchronization
 *
 * @param arg Gets interpreted as an int representing the thread ID.
 * @return NULL returned to satisfy requirements of POSIX Threads.
 */
void * consumer(void * arg) {
	// Unique integer ID is sent to each consumer thread
	int threadID = (intptr_t) arg; // intptr_t used to suppress a casting warning 
	// Announce that the thread has started executing
	printf("C%d entered\n", threadID); 
	int i;
	for(i = 0; i < CONSUMPTION_LIMIT; i++) { // Consume a set number of values


		sem_wait(&filled); // Decrement filled slots count
    	sem_wait(&semaphore); // Enter critical section
		// Start Critical Section
		int consumedResult = take(); // Take from buffer
		sem_post(&semaphore); // Exit critical section
    	sem_post(&empty); // Increment empty slots count
		consume(threadID, consumedResult); // Consume result 
		// End Critical Section: consume() appears here to assure sequential ordering of output

	}	
	// Announce completion of thread 
	printf("C%d finished\n", threadID); 
	return NULL; // No value is returned by thread
}

/**
 * main function initializes three producer threads and
 * two consumer threads.
 *
 * @return Success returns 0, crash/failure returns -1.
 */
int main() {
	printf("Producer/Consumer Program Launched\n");

	// Initialize semaphores
    sem_init(&empty, 0, BUFFER_SIZE); // Set empty slots count to buffer size
    sem_init(&filled, 0, 0); // Set filled slots count to 0
    sem_init(&semaphore, 0, 1); // Initialize mutex to 1

	pthread_t producerThread[NUM_PRODUCERS], consumerThread[NUM_CONSUMERS];
	int id;

	// Launch producers
	for(id = 1; id <= NUM_PRODUCERS; id++) {
		if(pthread_create(&producerThread[id - 1], NULL, &producer, (void*)(intptr_t) id)) {
			printf("Could not create thread\n");
			return -1;
		}
	}

	// Launch consumers
	for(id = 1; id <= NUM_CONSUMERS; id++) {
		if(pthread_create(&consumerThread[id - 1], NULL, &consumer, (void*)(intptr_t) id)) {
			printf("Could not create thread\n");
			return -1;
		}
	}

	printf("Threads initialized\n");

	// Producers complete
	for(id = 1; id <= NUM_PRODUCERS; id++) {
		if(pthread_join(producerThread[id - 1], NULL)) {
			printf("Could not join thread\n");
			return -1;
		}
	}

	// Consumers complete
	for(id = 1; id <= NUM_CONSUMERS; id++) {
		if(pthread_join(consumerThread[id - 1], NULL)) {
			printf("Could not join thread\n");
			return -1;
		}
	}

	return 0; // Successful termination
}
