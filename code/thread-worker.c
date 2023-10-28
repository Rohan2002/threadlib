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
    if (thread_id >= MAX_THREADS)
    {
        return NULL;
    }
    return thread_table[thread_id];
}

/* Add a new thread to the thread table */
void add_thread_to_thread_table(worker_t thread_id, tcb *tcb)
{
    if (thread_id >= MAX_THREADS)
    {
        return;
    }
    thread_table[thread_id] = tcb;
}

/* Remove a thread from the thread table */
void remove_thread_from_thread_table(worker_t thread_id)
{
    if (thread_id >= MAX_THREADS)
    {
        return;
    }
    thread_table[thread_id] = NULL;
}

void *inf_loop(void *args)
{
    int *count = malloc(sizeof(int));
    int *arg_pointer = args;

    *count = 0;
    while (1)
    {
        if (*count == (*arg_pointer * 10000))
        {
            break;
        }
        *count = *count + 1;
        printf("count t%d: %d\n", *arg_pointer, *count);
    }
    return count;
}

int safe_malloc(void **ptr, size_t size)
{
    if (ptr == NULL)
    {
        // avoid seg fault
        return ERROR_CODE;
    }
    *ptr = malloc(size);
    return *ptr != NULL;
}

void fall_back_to_schedular(int signum)
{
    // printf("fall back to scheduler\n");
    if (worker_yield() < 1)
    {
        // handle error
        perror("Failed to swap context with worker thread.");
        exit(1);
    }
};

void create_thread_timer()
{
    /**
     * Setup the timer.
     * Currently every 1 second interval, call fall_back_to_schedular function.
     */
    printf("Create thread timer\n");
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &fall_back_to_schedular;
    sigaction(SIGPROF, &sa, NULL);

    struct itimerval timer;
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec = 1;

    timer.it_value.tv_usec = 0;
    timer.it_value.tv_sec = 1; // TODO: QUantum variable
    setitimer(ITIMER_PROF, &timer, NULL);
    printf("Set timer function\n");
};

void wrapper_worker_function(void *(*function)(void *), void *arg)
{
    worker_exit(function(arg));
};

int _create_thread(tcb **thread_tcb_pointer, worker_t *thread_id)
{
    /**
     * Create the actual TCB on heap space.
     * Set thread status to READY.
     * Set thread ID.
     */
    if (!safe_malloc((void **)thread_tcb_pointer, sizeof(tcb)))
    {
        return ERROR_CODE;
    }
    tcb *thread_tcb = *thread_tcb_pointer;

    Threads_state ts = THREAD_READY;
    thread_tcb->status = ts;

    if (thread_id == NULL)
    {
        return ERROR_CODE;
    }

    thread_tcb->thread_id = *thread_id; // thread-id
    return 1;
};

int _populate_thread_context(tcb *thread_tcb)
{
    /**
     * populate the current tcb with the active context.
     */
    return getcontext(&thread_tcb->context) >= 0;
};

int _create_thread_context(tcb *thread_tcb, void *(*function)(void *), void *arg)
{
    /**
     * Create the thread's context.
     * Fill the context attribute with the active context
     * UC_LINK (successor context is NULL)
     * Create the thread stack.
     * Set stack attribute.
     */
    if (!_populate_thread_context(thread_tcb))
    {
        return ERROR_CODE;
    }
    thread_tcb->context.uc_link = NULL;

    if (!safe_malloc((void **)&(thread_tcb->context.uc_stack.ss_sp), STACK_SIZE))
    {
        return ERROR_CODE;
    }

    thread_tcb->context.uc_stack.ss_size = STACK_SIZE;
    thread_tcb->context.uc_stack.ss_flags = 0;

    // number of arguements
    if (thread_tcb->thread_id == MAIN_THREAD_ID || thread_tcb->thread_id == SCHEDULAR_THREAD_ID)
    {
        makecontext(&thread_tcb->context, (void (*)(void))function, 1, arg);
    }
    else
    {
        makecontext(&thread_tcb->context, (void (*)(void))wrapper_worker_function, 2, function, arg);
    }
    return 1;
};

/* create a new thread */
int worker_create(worker_t *worker_thread_id, pthread_attr_t *attr,
                  void *(*function)(void *), void *arg)
{
    /**
     * Steps to create a worker thread.
     *
     * We have 3 thread contexts created.
     * Main thread (create only once when first worker create is called)
     * Schedular thread (create only once when first worker create is called)
     * Worker thread
     *
     * Setup the timer to switch back to the schedular function
     * Setup the queue to store the active threads
     *
     * Create the worker thread.
     * Enqueue(worker thread)
     */

    // first time init only
    if (firstTimeWorkerThread)
    {
        tcb *main_thread;
        worker_t main_thread_id = MAIN_THREAD_ID;
        if (!_create_thread(&main_thread, &main_thread_id))
        {
            return ERROR_CODE;
        }
        // main thread context is set by getcontext
        setCurrentThread(main_thread);

        add_thread_to_thread_table(main_thread_id, main_thread);

        // thread schedular config.
        tcb *schedular_thread;
        worker_t schedular_thread_id = SCHEDULAR_THREAD_ID;
        if (!_create_thread(&schedular_thread, &schedular_thread_id))
        {
            return ERROR_CODE;
        }
        // create thread context for worker thread.
        if (!_create_thread_context(schedular_thread, schedule_entry_point, NULL))
        {
            return ERROR_CODE;
        }
        setSchedularThread(schedular_thread);
        add_thread_to_thread_table(schedular_thread_id, schedular_thread);

        // create thread runqueue and timer
        queue_t *q = create_queue();
        setThreadQueue(q);
        create_thread_timer();

        firstTimeWorkerThread = 0;
    }

    // worker thread.
    tcb *worker_thread;
    if (!_create_thread(&worker_thread, worker_thread_id))
    {
        return ERROR_CODE;
    }
    if (!_create_thread_context(worker_thread, function, arg))
    {
        return ERROR_CODE;
    }
    add_thread_to_thread_table(*worker_thread_id, worker_thread);

    // enqueue worker thread.
    enqueue(getThreadQueue(), worker_thread);

    return worker_thread->thread_id;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{
    /**
     * There should be a current thread.
     * There should be a schedular thread.
     *
     * Set current thread to active only if it is not finished.
     *
     * change context from worker thread to schedular thread.
     */
    // printf("Entered the worker yield function\n");
    if (getCurrentThread() == NULL)
    {
        printf("Current thread is null\n");
        return ERROR_CODE;
    }
    if (getSchedularThread() == NULL)
    {
        printf("Schedular thread is null\n");
        return ERROR_CODE;
    }
    tcb *current_thread = getCurrentThread();
    tcb *schedular_thread = getSchedularThread();

    if (!thread_finished(current_thread))
    {
        current_thread->status = THREAD_READY; // interuppted there
    }

    // setCurrentThread(current_thread);
    // printf("Before swap context to schedular Current thread ID: %d status: %d\n", getCurrentThread()->thread_id, getCurrentThread()->status);
    if (swapcontext(&(current_thread->context), &(schedular_thread->context)) < 0)
    {
        printf("Cannot exec anymore\n");
        return ERROR_CODE;
    }
    // printf("Swapping context back to the main thread\n");
    return 1;
};

/* terminate a thread */
void worker_exit(void *value_ptr)
{
    tcb *current_thread = getCurrentThread();

    current_thread->ret_val = value_ptr;
    current_thread->status = THREAD_FINISHED;

    swapcontext(&current_thread->context, &getSchedularThread()->context);
};

tcb *get_main_thread()
{
    return find_tcb(MAIN_THREAD_ID);
}

int thread_finished(tcb *thread)
{
    return thread->status == THREAD_FINISHED;
}

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{
    /* Wait for thread termination */
    tcb *thread_tcb;

    // Find the TCB for the specified thread ID
    thread_tcb = find_tcb(thread);
    if (thread_tcb == NULL)
    {
        return ERROR_CODE;
    }
    while (thread_tcb->status != THREAD_FINISHED)
    {
        worker_yield();
    }
    if (value_ptr != NULL)
    {
        *value_ptr = thread_tcb->ret_val;
    }
    if (&thread_tcb->context.uc_stack != NULL)
    {
        free(thread_tcb->context.uc_stack.ss_sp);
    }
    free(thread_tcb);
    return 1;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
                      const pthread_mutexattr_t *mutexattr)
{
    //- initialize data structures for this mutex

    // YOUR CODE HERE

    mutex->locked = 0;                  // Indicates that mutex is unlocked initially. 0 for unlocked, 1 for locked.
    mutex->owner = NULL;                // No owner yet because it's unlocked
    mutex->block_list = create_queue(); // A queue to manage threads waiting for this mutex
    return ERROR_CODE;
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
    while (__sync_lock_test_and_set(&mutex->locked, 1))
    {
        // Mutex has been locked, we will add the current thread to the list of threads waiting for unlock
        enqueue(mutex->block_list, getCurrentThread());
        // Context switch to the scheduler to run other threads
        schedule(); // This should save the current state and switch to scheduler context (!!Not implemented yet!!)
    }

    // If we've reached here, it means we've acquired the lock
    mutex->owner = getCurrentThread(); // The current thread is now the owner of the mutex

    return ERROR_CODE;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
    // - release mutex and make it available again.
    // - put threads in block list to run queue
    // so that they could compete for mutex later.

    // YOUR CODE HERE

    if (mutex->owner != getCurrentThread())
    {
        // The current thread is trying to unlock a mutex it doesn't own
        return ERROR_CODE; // Return an error condition
    }

    // Clear the owner and unlock
    mutex->owner = NULL;
    __sync_lock_release(&mutex->locked); // Unlock mutex by setting to 0

    // If there are threads waiting for this mutex, time to wake them up
    if (!is_empty(mutex->block_list))
    {
        // Move all threads from block list to run queue, they're ready to run
        while (!is_empty(mutex->block_list))
        {
            // Add the thread to the ready queue, so scheduler can run it
            tcb *next_thread = dequeue(mutex->block_list);
            enqueue(getThreadQueue(), next_thread);
        }
    }

    return ERROR_CODE;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
    // - de-allocate dynamic memory created in worker_mutex_init

    // if mutex is locked, we cannot destroy it as it will keep the list blocked.
    if (mutex->locked)
    {
        return ERROR_CODE;
    }

    destroy_queue(mutex->block_list);
    return ERROR_CODE;
};

/* scheduler */
void *schedule_entry_point(void *args)
{
    schedule();
    return NULL;
}

static void schedule()
{
    // sched_psjf();
    sched_mlfq();
    // - every time a timer interrupt occurs, your worker thread library
    // should be contexted switched from a thread context to this
    // schedule() function

    // - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

    // if (sched == PSJF)
    // 		sched_psjf();
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

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
// static int sched_psjf()
// {
//     printf("Come back to schedular\n");
//     while (1)
//     {
//         if (getCurrentThread() != NULL && !thread_finished(getCurrentThread()))
//         {
//             printf("Enqueue running thread id: %d\n", getCurrentThread()->thread_id);
//             enqueue(getThreadQueue(), getCurrentThread());
//             // thread got interrupted. enqueue again.
//         }
//         printf("Finding next thread to schedule\n");
//         if (is_empty(getThreadQueue()))
//         {
//             printf("Main thread exited unexpectedly! Killing main process.");
//             exit(1); // completely destroys process
//         }
//         // worker thread exec.
//         if (!is_empty(getThreadQueue()))
//         {
//             tcb *thread_to_run = dequeue(getThreadQueue());
//             Threads_state ts = THREAD_RUNNING;
//             thread_to_run->status = ts;

//             printf("Dequeued running thread id: %d\n", thread_to_run->thread_id);

//             setCurrentThread(thread_to_run);
//             printf("Swapping context to thread id: %d\n", getCurrentThread()->thread_id);
//             if (getCurrentThread()->thread_id == MAIN_THREAD_ID)
//             {
//                 printf("&getCurrentThread()->context: {%p}\n", &getCurrentThread()->context);
//                 setcontext(&getCurrentThread()->context);
//             }
//             if (swapcontext(&getSchedularThread()->context, &getCurrentThread()->context) < 0)
//             {
//                 printf("Swap context failed\n");
//                 return ERROR_CODE;
//             }
//             printf("After context swap\n");
//         }
//     }
// }


#define NUM_PRIORITY_LEVELS 4
#define TIME_QUANTUM 100

/* Preemptive MLFQ scheduling algorithm */
static int sched_mlfq()
{
    printf("Come back to scheduler\n");

    // Define multiple priority levels
    queue_t *thread_queues[NUM_PRIORITY_LEVELS];
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        thread_queues[i] = create_queue();
    }

    int time_since_last_promotion = 0;

    while (1)
    {
        // Check if current thread is finished or interrupted
        if (getCurrentThread() != NULL && !thread_finished(getCurrentThread()))
        {
            // Enqueue thread in appropriate priority level
            int priority = getCurrentThread()->priority;
            enqueue(thread_queues[priority], getCurrentThread());
        }

        // Find next thread to schedule
        printf("Finding next thread to schedule\n");
        tcb *thread_to_run = NULL;
        for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
            if (!is_empty(thread_queues[i])) {
                thread_to_run = dequeue(thread_queues[i]);
                break;
            }
        }

        // If no threads are ready to run, exit
        if (thread_to_run == NULL) {
            printf("Main thread exited unexpectedly! Killing main process.");
            exit(1);
        }

        // Set thread status to running
        Threads_state ts = THREAD_RUNNING;
        thread_to_run->status = ts;

        // Swap context to next thread
        setCurrentThread(thread_to_run);
        printf("Swapping context to thread id: %d\n", getCurrentThread()->thread_id);
        if (getCurrentThread()->thread_id == MAIN_THREAD_ID)
        {
            printf("&getCurrentThread()->context: {%p}\n", &getCurrentThread()->context);
            setcontext(&getCurrentThread()->context);
        }
        if (swapcontext(&getSchedularThread()->context, &getCurrentThread()->context) < 0)
        {
            printf("Swap context failed\n");
            return ERROR_CODE;
        }
        printf("After context swap\n");

        // Update time since last promotion
        time_since_last_promotion += TIME_QUANTUM;

        // Implement aging
        if (time_since_last_promotion >= 500) {
            for (int i = 1; i < NUM_PRIORITY_LEVELS; i++) {
                tcb *thread = NULL;
                while (!is_empty(thread_queues[i])) {
                    thread = dequeue(thread_queues[i]);
                    thread->priority--;
                    enqueue(thread_queues[i-1], thread);
                }
            }
            time_since_last_promotion = 0;
        }

        // Implement preemption
        if (getCurrentThread() != NULL && getCurrentThread()->time_running >= TIME_QUANTUM) {
            getCurrentThread()->time_running = 0;
            int priority = getCurrentThread()->priority;
            if (priority < NUM_PRIORITY_LEVELS - 1) {
                enqueue(thread_queues[priority+1], getCurrentThread());
            } else {
                enqueue(thread_queues[priority], getCurrentThread());
            }
            thread_to_run = NULL;
        }

        // Update time running for current thread
        if (getCurrentThread() != NULL) {
            getCurrentThread()->time_running += TIME_QUANTUM;
        }
    }
}

// DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void)
{

    fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
    fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
    fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}

int main(int argc, char **argv)
{
    worker_t *tid_pointer2 = malloc(sizeof(worker_t));
    *tid_pointer2 = 2;
    worker_t *tid_pointer3 = malloc(sizeof(worker_t));
    *tid_pointer3 = 3;

    int *args = (int *)calloc(5, sizeof(int));
    args[1] = 2;

    int *i1 = malloc(sizeof(int));
    int *i2 = malloc(sizeof(int));
    *i1 = 4;
    *i2 = 5;

    int tid0 = worker_create(tid_pointer2, NULL, inf_loop, i1);
    int tid1 = worker_create(tid_pointer3, NULL, inf_loop, i2);

    printf("Created tid: %d\n", tid0);
    printf("Created tid: %d\n", tid1);

    void *tid0_ret = malloc(sizeof(int));
    void *tid1_ret = malloc(sizeof(int));

    worker_join(tid0, &tid0_ret);
    worker_join(tid1, &tid1_ret);
    printf("Main thread tid0 ret val: %d\n", *(int *)tid0_ret);
    printf("Main thread tid0 ret val: %d\n", *(int *)tid1_ret);

    // while (1);
    free(args);
    free(tid_pointer2);
    free(tid_pointer3);
    printf("Reached this control block\n");
    return EXIT_SUCCESS;

    // simulate timer.
    // create_thread_timer();
    // while(1);
}

void setCurrentThread(tcb *thread_exec)
{
    current_thread = thread_exec;
}

tcb *getCurrentThread()
{
    return current_thread;
}

void setSchedularThread(tcb *thread_exec)
{
    schedular_thread = thread_exec;
}

tcb *getSchedularThread()
{
    return schedular_thread;
}

void setThreadQueue(queue_t *q)
{
    thread_queue = q;
}

queue_t *getThreadQueue()
{
    return thread_queue;
}