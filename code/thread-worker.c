// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include <stdio.h>
#include <stdlib.h>
#include "thread-worker.h"
#include <ucontext.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

// Global counter for total context switches and
// average turn around and response time
long tot_cntx_switches = 0;
double avg_turn_time = 0;
double avg_resp_time = 0;

#define STACK_SIZE SIGSTKSZ

tcb *current_thread = NULL;

void *simplef(void *args)
{
    int *p = args;
    for (int i = 0; i < 5; i++)
    {
        printf("Arg: %d\n", p[i]);
    }
    puts("Donald- you are threaded\n");
    return NULL;
}

int safe_malloc(void** ptr, size_t size){
    if(ptr == NULL){
        // avoid seg fault
        return 0;
    }
    *ptr = malloc(size);
    return *ptr != NULL;
}

int populate_thread_context(tcb* thread_tcb){
    return getcontext(&(thread_tcb)->context) >= 0;
}

int _create_thread_context(tcb *thread_tcb, void *(*function)(void *), void *arg)
{
    if(!populate_thread_context(thread_tcb)){
        return 0;
    }

    thread_tcb->context.uc_link = NULL;

    if(!safe_malloc((void**)&(thread_tcb->context.uc_stack.ss_sp), STACK_SIZE)){
        return 0;
    }

    thread_tcb->context.uc_stack.ss_size = STACK_SIZE;
    thread_tcb->context.uc_stack.ss_flags = 0;

    makecontext(&thread_tcb->context, (void (*)(void))(function), 1, arg);
    return 1;
}

int _create_thread(tcb **thread_tcb_pointer, worker_t *thread_id){
    if(!safe_malloc((void**)thread_tcb_pointer, sizeof(tcb))){
        return 0;
    }
    tcb* thread_tcb = *thread_tcb_pointer;

    Threads_state ts = THREAD_READY;
    thread_tcb->status = ts;

    if(thread_id == NULL){
        return 0;
    }

    thread_tcb->thread_id = *thread_id;
    return 1;
};

/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr,
                  void *(*function)(void *), void *arg)
{

    // - create Thread Control Block (TCB)
    // - create and initialize the context of this worker thread
    // - allocate space of stack for this thread to run
    // after everything is set, push this thread into run queue and
    // - make it ready for the execution.

    tcb *worker_thread;
    if(!_create_thread(&worker_thread, thread)){
        return 0;
    }

    // create thread context for worker thread.
    if(!_create_thread_context(worker_thread, function, arg)){
        return 0;
    }

    if(!setcontext(&worker_thread->context)){
        return 0;
    };

    return 1;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{

    // - change worker thread's state from Running to Ready
    // - save context of this thread to its thread control block
    // - switch from thread context to scheduler context

    // YOUR CODE HERE

    return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr){
    // de-allocate any dynamic memory created when starting this thread
    tcb* p = value_ptr;
    if(&p->context.uc_stack != NULL){
        free(p->context.uc_stack.ss_sp);
    }
    free(p);
};

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{

    // - wait for a specific thread to terminate
    // - de-allocate any dynamic memory created by the joining thread

    // YOUR CODE HERE
    return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
                      const pthread_mutexattr_t *mutexattr)
{
    //- initialize data structures for this mutex

	// YOUR CODE HERE

	mutex->locked = 0;  // Indicates that mutex is unlocked initially. 0 for unlocked, 1 for locked.
    mutex->owner = NULL; // No owner yet because it's unlocked
    mutex->block_list = create_queue(); // A queue to manage threads waiting for this mutex
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex)
{

    // - use the built-in test-and-set atomic function to test the mutex
    // - if the mutex is acquired successfully, enter the critical section
    // - if acquiring mutex fails, push current thread into block list and
    // context switch to the scheduler thread

        // YOUR CODE HERE

		// using the built-in atomic test set function in a loop to test the mutex
		while (__sync_lock_test_and_set(&mutex->locked, 1)) {
			// Mutex has been locked, we will add the current thread to the list of threads waiting for unlock
			enqueue(mutex->block_list, getCurrentThread());
			// Context switch to the scheduler to run other threads
			schedule();  // This should save the current state and switch to scheduler context (!!Not implemented yet!!)
		}
		
		// If we've reached here, it means we've acquired the lock
		mutex->owner = getCurrentThread();  // The current thread is now the owner of the mutex
		
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
    // - release mutex and make it available again.
    // - put threads in block list to run queue
    // so that they could compete for mutex later.

	// YOUR CODE HERE

	if (mutex->owner != getCurrentThread()) {
        // The current thread is trying to unlock a mutex it doesn't own
        return -1;  // Return an error condition
    }
    
    // Clear the owner and unlock
    mutex->owner = NULL;
    __sync_lock_release(&mutex->locked);  // Unlock mutex by setting to 0
    
    // If there are threads waiting for this mutex, time to wake them up
    if (!is_empty(mutex->block_list)) {
        // Move all threads from block list to run queue, they're ready to run
        while (!is_empty(mutex->block_list)) {
            tcb* next_thread = dequeue(mutex->block_list);
            // Add the thread to the ready queue, so scheduler can run it
            //Ready Queue not implemented.
        }
    }

	return 0;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
    // - de-allocate dynamic memory created in worker_mutex_init

    //if mutex is locked, we cannot destroy it as it will keep the list blocked.
    if (mutex->locked){
        return -1;
    }

    destroy_queue(mutex->block_list);
	return 0;
};

// /* scheduler */
// static void schedule() {
// 	// - every time a timer interrupt occurs, your worker thread library
// 	// should be contexted switched from a thread context to this
// 	// schedule() function

// 	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

// 	// if (sched == PSJF)
// 	//		sched_psjf();
// 	// else if (sched == MLFQ)
// 	// 		sched_mlfq();

// 	// YOUR CODE HERE

// // - schedule policy
// #ifndef MLFQ
// 	// Choose PSJF
// #else
// 	// Choose MLFQ
// #endif

// }

// /* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
// static void sched_psjf() {
// 	// - your own implementation of PSJF
// 	// (feel free to modify arguments and return types)

// 	// YOUR CODE HERE
// }

// /* Preemptive MLFQ scheduling algorithm */
// static void sched_mlfq() {
// 	// - your own implementation of MLFQ
// 	// (feel free to modify arguments and return types)

// 	// YOUR CODE HERE
// }

// DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void)
{

    fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
    fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
    fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}

// Feel free to add any other functions you need

// YOUR CODE HERE

int main(int argc, char **argv)
{
    worker_t *tid_pointer = malloc(sizeof(worker_t));
    *tid_pointer = 1;
    int *args = (int *)calloc(5, sizeof(int));
    args[1] = 2;
    int tid = worker_create(tid_pointer, NULL, simplef, args);
    printf("Created tid: %d\n", tid);
    printf("Hello\n");
    free(args);
}

// queue_t holds reference to the front and rear node of the queue
queue_t *create_queue()
{
    queue_t *q = malloc(sizeof(queue_t));
    if (q == NULL)
    {
        perror("Unable to allocate memory for new queue");
        exit(EXIT_FAILURE); // Exit the program as we couldn't create the queue
    }
    q->front = q->rear = NULL;
    return q;
}

// adds element to the queue
void enqueue(queue_t *q, int value)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node == NULL)
    {
        perror("Unable to allocate memory for new node");
        exit(EXIT_FAILURE); // Exit the program as we couldn't enqueue new item
    }
    new_node->data = value;
    new_node->next = NULL;

    // If queue is empty, the new node is both the front and the rear
    if (q->rear == NULL)
    {
        q->front = q->rear = new_node;
        return;
    }

    // Add the new node at the end of the queue and change rear
    q->rear->next = new_node;
    q->rear = new_node;
}

int dequeue(queue_t *q)
{
    // If queue is empty, return a special value or error
    if (q->front == NULL)
    {
        fprintf(stderr, "Queue is empty, unable to dequeue\n");
        return -1; // Or another special value to indicate error
    }

    // Store previous front and move front one node ahead
    node_t *temp_node = q->front;
    int item = temp_node->data;
    q->front = q->front->next;

    // If front becomes NULL, then change rear also to NULL
    if (q->front == NULL)
    {
        q->rear = NULL;
    }

    free(temp_node);
    return item;
}

// Function to check if the queue is empty
int is_empty(queue_t *q)
{
    return q->front == NULL;
}

// Function to get the front of the queue
int front(queue_t *q)
{
    if (q->front == NULL)
    {
        fprintf(stderr, "Queue is empty\n");
        return -1; // Or another special value to indicate error
    }
    return q->front->data;
}

// Function to free the queue and its nodes
void destroy_queue(queue_t *q)
{
    node_t *current = q->front;
    node_t *next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    free(q);
}

tcb* getCurrentThread() {
    return current_thread;
}