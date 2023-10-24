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

void *simplef(void *args)
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
        printf("count: %d", count++);
    }
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
    worker_yield();
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
	timer.it_value.tv_sec = 1;

	setitimer(ITIMER_PROF, &timer, NULL);
}

int _populate_thread_context(tcb* thread_tcb){
    return getcontext(&(thread_tcb)->context) >= 0;
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

    // init thread queue
    if (getThreadQueue() == NULL){
        queue_t *q = create_queue();
        setThreadQueue(q);
    }

    // init schedular only for the first time
    if (getSchedularThread() == NULL){
        tcb *schedular_thread;
        if(!_create_thread(&schedular_thread, thread)){
            return 0;
        }
        // create thread context for worker thread.
        if(!_create_thread_context(schedular_thread, schedule_entry_point, NULL)){
            return 0;
        }
        setSchedularThread(schedular_thread);
        create_thread_timer();
    }

    // worker thread
    tcb *worker_thread;
    if(!_create_thread(&worker_thread, thread)){
        return 0;
    }

    // create thread context for worker thread.
    if(!_create_thread_context(worker_thread, function, arg)){
        return 0;
    }
    setCurrentThread(worker_thread);
    enqueue(getThreadQueue(), worker_thread); 
    return 1;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{
    // - change worker thread's state from Running to Ready
    // - save context of this thread to its thread control block
    // - switch from thread context to scheduler context
    if (getCurrentThread() == NULL || getSchedularThread() == NULL){
        return 0;
    }
    tcb* current_thread = getCurrentThread();
    tcb* schedular_thread = getSchedularThread();

    current_thread->status = THREAD_READY;

    setCurrentThread(current_thread);
    swapcontext(&(current_thread->context), &(schedular_thread->context));

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
            // tcb* next_thread = dequeue(mutex->block_list);
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
    if (getCurrentThread() != NULL){
        enqueue(getThreadQueue(), getCurrentThread());
    }
    printf("Finding next thread to schedule\n");
    if(!is_empty(getThreadQueue())){
        tcb* thread_to_run = dequeue(getThreadQueue());
        Threads_state ts = THREAD_RUNNING;
        thread_to_run->status = ts;
        setCurrentThread(thread_to_run);
        swapcontext(&(getSchedularThread()->context), &(getCurrentThread())->context);
    }
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

int main(int argc, char **argv)
{
    worker_t *tid_pointer = malloc(sizeof(worker_t));
    *tid_pointer = 1;
    int *args = (int *)calloc(5, sizeof(int));
    args[1] = 2;
    int tid = worker_create(tid_pointer, NULL, simplef, args);
    printf("Created tid: %d\n", tid);
    free(args);
    free(tid_pointer);

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