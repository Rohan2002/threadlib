// File:	worker_t.h

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

typedef uint worker_t;
typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_FINISHED 
} thread_status;

typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	worker_t thread_id;
	ucontext_t context;
	void* stack;
	thread_status status;
	int priority;
} tcb; 

// Node structure for the queue
typedef struct node {
    int data; // Change this type if you need to store something else
    struct node* next;
} node_t;

// Queue structure
typedef struct queue {
    node_t* front; // Points to the front node in the queue
    node_t* rear;  // Points to the rear node in the queue
} queue_t;

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */

	// YOUR CODE HERE

	volatile int locked;  // Flag indicating whether the mutex is locked (1) or unlocked (0)
    tcb *owner;           // Pointer to the TCB of the owning thread
    queue_t *block_list;  // Queue of TCBs of threads blocked waiting for this mutex

} worker_mutex_t;


/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

tcb* getCurrentThread();

/* Function Declarations: */

//for implementing Queue data structure

queue_t* create_queue();
void enqueue(queue_t* q, int value);
int dequeue(queue_t* q);
int is_empty(queue_t* q);
int front(queue_t* q);
void destroy_queue(queue_t* q);



/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);


/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
