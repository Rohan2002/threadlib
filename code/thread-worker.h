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
#include <signal.h>
#include <queue.h>

typedef uint worker_t;

// util functions
int safe_malloc(void** ptr, size_t size);

// thread variables and data structures.
int QUANTUM = 1; // seconds
typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_FINISHED 
} Threads_state;

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
	Threads_state status;
	int priority;
} tcb; 
int _populate_thread_context(tcb* thread_tcb);
int _create_thread_context(tcb *thread_tcb, void *(*function)(void *), void *arg);
int _create_thread(tcb **thread_tcb_pointer, worker_t *thread_id);
void create_thread_timer();

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */

	// YOUR CODE HERE

	volatile int locked;  // Flag indicating whether the mutex is locked (1) or unlocked (0)
    tcb *owner;           // Pointer to the TCB of the owning thread
    queue_t *block_list;  // Queue of TCBs of threads blocked waiting for this mutex

} worker_mutex_t;


void setCurrentThread(tcb* thread_exec);
tcb* getCurrentThread();

void setSchedularThread(tcb* thread_exec);
tcb* getSchedularThread();

void setThreadQueue(queue_t* q);

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

/* Scheduler */
typedef struct sigaction signal_type;
void *schedule_entry_point(void* args);
static void schedule();

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
