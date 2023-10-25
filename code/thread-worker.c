// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "thread-worker.h"


// Global counter for total context switches and
// average turn around and response time
long tot_cntx_switches = 0;
double avg_turn_time = 0;
double avg_resp_time = 0;

#define STACK_SIZE SIGSTKSZ

tcb *current_thread = NULL;
tcb *schedular_thread = NULL;

queue_t *thread_queue = NULL;

int firstTimeWorkerThread = 1;
tcb *thread_table[MAX_THREADS];
/* Find the TCB for the specified thread ID */
tcb *find_tcb(worker_t thread_id)
{
    if (thread_id >= MAX_THREADS) {
        return NULL;
    }
    return thread_table[thread_id];
}

/* Add a new thread to the thread table */
void add_thread_to_thread_table(worker_t thread_id, tcb *tcb)
{
    if (thread_id >= MAX_THREADS) {
        return;
    }
    thread_table[thread_id] = tcb;
}

/* Remove a thread from the thread table */
void remove_thread_from_thread_table(worker_t thread_id)
{
    if (thread_id >= MAX_THREADS) {
        return;
    }
    thread_table[thread_id] = NULL;
}

void *inf_loop(void *args)
{
    // int *p = args;
    // for (int i = 0; i < 5; i++)
    // {
    //     printf("Arg: %d\n", p[i]);
    // }
    // puts("Donald- you are threaded\n");
    // return NULL;
    int count = 0;
    while(1){
        printf("count t1: %d\n", count++);
    }
    return NULL;
}
void *immediate_return(void *args)
{
    // int *p = args;
    // for (int i = 0; i < 5; i++)
    // {
    //     printf("Arg: %d\n", p[i]);
    // }
    // puts("Donald- you are threaded\n");
    // return NULL;
    int count = 0;
    while(1){
        if (count == 10){
            break;
        }
        printf("count t2: %d\n", count++);
    }
    // while(1);
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

void fall_back_to_schedular(int signum){
	printf("fall back to scheduler\n");
    int worker_status = worker_yield();
    printf("worker_status: %d\n", worker_status);
}

void create_thread_timer(){
    printf("Create thread timer\n");
    struct sigaction sa;

    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = &fall_back_to_schedular;
	sigaction (SIGPROF, &sa, NULL);

    struct itimerval timer;
    timer.it_interval.tv_usec = 0; 
	timer.it_interval.tv_sec = 1;

	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 1;  // TODO: QUantum variable
	setitimer(ITIMER_PROF, &timer, NULL);
    printf("Set timer function\n");
}

int _populate_thread_context(tcb* thread_tcb){
    return getcontext(&thread_tcb->context) >= 0;
}

int _create_thread_context(tcb *thread_tcb, void *(*function)(void *), void *arg)
{
    if(!_populate_thread_context(thread_tcb)){
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

    thread_tcb->thread_id = *thread_id; // thread-id
    return 1;
};

/* create a new thread */
int worker_create(worker_t *worker_thread_id, pthread_attr_t *attr,
                  void *(*function)(void *), void *arg)
{
    if(firstTimeWorkerThread){
        tcb *main_thread;
        worker_t main_thread_id = 0;
        if(!_create_thread(&main_thread, &main_thread_id)){
            return -1;
        }
        // main thread context is set by getcontext
        setCurrentThread(main_thread);
        // create thread runqueue
        queue_t *q = create_queue();
        setThreadQueue(q);

        // thread schedular config.
        tcb *schedular_thread;
        worker_t schedular_thread_id = 1;
        if(!_create_thread(&schedular_thread, &schedular_thread_id)){
            return -1;
        }
        // create thread context for worker thread.
        if(!_create_thread_context(schedular_thread, schedule_entry_point, NULL)){
            return -1;
        }
        setSchedularThread(schedular_thread);
        create_thread_timer();

        firstTimeWorkerThread = 0;
    }

    // worker thread. Just put it in the queue and don't do anything.
    tcb *worker_thread;
    if(!_create_thread(&worker_thread, worker_thread_id)){
        return -1;
    }

    //indexes the thread in the thread table so we can reference it with thread_id
    thread_table[*worker_thread_id] = worker_thread;
    // create thread context for worker thread.
    if(!_create_thread_context(worker_thread, function, arg)){
        return -1;
    }

    enqueue(getThreadQueue(), worker_thread); 
    // printf("Vanilla worker thread run\n");
    // setcontext(&getCurrentThread()->context);
    return worker_thread->thread_id;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{   
    printf("In the worker yield function\n");
    // - change worker thread's state from Running to Ready
    // - save context of this thread to its thread control block
    // - switch from thread context to scheduler context
    if (getCurrentThread() == NULL){
        printf("Current thread is null\n");
        return 0;
    }
    if(getSchedularThread() == NULL){
        printf("Schedular thread is null\n");
        return 0;
    }
    tcb* current_thread = getCurrentThread();
    tcb* schedular_thread = getSchedularThread();

    current_thread->status = THREAD_READY; // interuppted thre

    printf("Before going to schedular, the active thread id is: %d\n", current_thread->thread_id);
    printf("Before swap context Current thread ID: %d status: %d\n",current_thread->thread_id, current_thread->status);
    if (swapcontext(&(current_thread->context), &(schedular_thread->context)) < 0){
        printf("Cannot exec anymore\n");
        return 0;
    }
    printf("After swap context Current thread ID: %d status: %d\n",current_thread->thread_id, current_thread->status);
    printf("Swap context called in worker_yield\n");
    return 1;
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

    /* Wait for thread termination */
    tcb* thread_tcb;
    // Find the TCB for the specified thread ID
    thread_tcb = find_tcb(thread);

    // Wait for the thread to terminate
    while (thread_tcb->status != THREAD_FINISHED) {
        worker_yield();
    }

    // Deallocate any dynamic memory created by the joining thread
    worker_exit(thread_tcb);

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
            // Add the thread to the ready queue, so scheduler can run it
            tcb* next_thread = dequeue(mutex->block_list);
            enqueue(getThreadQueue(), next_thread); 

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

/* scheduler */
void* schedule_entry_point(void* args){
    schedule();
    return NULL;
}

static void schedule() {
    sched_psjf();
	// - every time a timer interrupt occurs, your worker thread library
	// should be contexted switched from a thread context to this
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

    // - schedule policy
    // #ifndef MLFQ
    //     // Choose PSJF
    // #else
    //     // Choose MLFQ
    // #endif

}

// /* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
    printf("Come back to schedular\n");
    while(1){
        printf("comaback\n");
        if (getCurrentThread() != NULL){
            printf("Enqueue running thread id: %d\n", getCurrentThread()->thread_id);
            enqueue(getThreadQueue(), getCurrentThread());
            // thread got interrupted. enqueue again.
        }
        printf("Finding next thread to schedule\n");
        if(is_empty(getThreadQueue())){
            printf("Main thread exited unexpectedly! Killing main process.");
            exit(1); // completely destroys process
        }
        // worker thread exec.
        if(!is_empty(getThreadQueue())){
            tcb* thread_to_run = dequeue(getThreadQueue());
            Threads_state ts = THREAD_RUNNING;
            thread_to_run->status = ts; 
            printf("Dequeued running thread id: %d\n", thread_to_run->thread_id);

            setCurrentThread(thread_to_run);
            printf("Swapping context to worker\n");
            printf("Thread ID: %d\n", getCurrentThread()->thread_id);
            if(swapcontext(&getSchedularThread()->context, &getCurrentThread()->context) < 0){
                printf("Swap context failed\n");
                return;
            }
            printf("After context swap\n");
        }
    }
    printf("I am at the end of the loop\n");
}

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

// crt.o

int main(int argc, char **argv)
{
    worker_t *tid_pointer2 = malloc(sizeof(worker_t));
    *tid_pointer2 = 2;
    worker_t *tid_pointer3 = malloc(sizeof(worker_t));
    *tid_pointer3 = 3;

    int *args = (int *)calloc(5, sizeof(int));
    args[1] = 2;

    int tid0 = worker_create(tid_pointer2, NULL, inf_loop, args);
    int tid1 = worker_create(tid_pointer3, NULL, immediate_return, args);
    
    printf("Created tid: %d\n", tid0);
    printf("Created tid: %d\n", tid1);
    
    free(args);
    free(tid_pointer2);
    free(tid_pointer3);
    while(1);
    printf("Reached this control block\n");
    return EXIT_SUCCESS;


    // simulate timer.
    // create_thread_timer();
    // while(1);
}


void setCurrentThread(tcb* thread_exec){
    current_thread = thread_exec;
}

tcb* getCurrentThread() {
    return current_thread;
}

void setSchedularThread(tcb* thread_exec){
    schedular_thread = thread_exec;
}

tcb* getSchedularThread() {
    return schedular_thread;
}

void setThreadQueue(queue_t* q){
    thread_queue = q;
}

queue_t* getThreadQueue() {
    return thread_queue;
}