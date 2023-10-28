# CS416 Project 2: User-level Thread Library and Scheduler

Rohan Deshpande (ryd4)
Jinyue (Eric) Liu (jl2661)

## Compile Steps

Run ```make SCHED=PSJF``` inside the code directory to compile the static threadlibrary. 

Go to benchmark folder and follow the steps in the README.md file provided initially.

## Part 1. Thread Library

### Thread creation 

Worker_Create will also be responsible for setting up the scheduler/main context the first time it is invoked, we used a global variable to track this initialization process. Afterward we will initialize a new thread and put it inside the runqueue waiting for execution.

### Thread yield

The function first checks if there is a current thread and a scheduler thread. If either of these threads is NULL, the function returns an error code.

If there is a current thread, the function sets its status to THREAD_READY if it has not finished executing.

The function then swaps the context from the current thread to the scheduler thread using the swapcontext() function. This effectively yields the current thread and switches to the scheduler thread.

After the context switch is complete, the function returns 1 to indicate success.


### Thread join

The input of the thread join will be the thread we wait for to finish, while we are waiting, we will yield to the scheduler to make other functions 

### Mutexes
The worker_mutex_init() function is used to setup the datastructure for the mutex, they include

- a lock status (locked) to demonstrate whether the mutex is currently locked or not
- owner field, to assign a thread as the mutex's owner, and to prevent any other thread from accessing the locked resource
- blocked_list, which holds the list of threads that are blocked from accessing the resource

worker_mutex_lock() is the function to lock the mutex, apart from setting the mutex status to lock, we used the linux atmoic function "__sync_lock_test_and_set" to set up the lock. At the same time, any other threads attempting to access the locked resource will have its status changed to "BLOCKED" and throw into the mutex->block_list queue, waiting to be called once the resource is unlocked

worker_mutex_unlock() is the reverse of the above mentioned process and used "__sync_lock_release" to release the lock, distributing every blocked thread back to the scheduler

worker_mutex_destroy() will free the mutex only when its initialized and unlocked. it will also free the entire queue associated.

## Part 2. Scheduler

### PSJF Scheduler

the Preemptive Shortest Job First (PSJF) scheduling algorithm in thread-worker.c.

The function first checks if there is a current thread and if it has finished executing or is blocked. If the current thread is still running, it is enqueued in the thread queue.

The function then dequeues the next thread to execute from the thread queue based on its remaining burst time. If the thread queue is empty, the function exits the program.

The dequeued thread is set to THREAD_RUNNING status and its response time is computed if it has not been run before. The function then swaps the context to the dequeued thread using swapcontext().

After the context switch is complete, the function returns to the scheduler thread, which enqueues the current thread if it is still running and repeats the process of dequeuing the next thread to execute.

This implementation of the PSJF scheduling algorithm is preemptive, meaning that the currently executing thread can be preempted if a new thread with a shorter burst time arrives.

### MLFQ Scheduler

The implementation of MLFQ scheduelr starts with an array of queues, where the index, from low to high, represents the priority of the queue, from top priority to low priority.

thread_queue[0] is the higher priority queue than thread_queue[3], for example.

When the thread is either interrupted or yielded, we sends them back to either the same priority queue or one priority lower, depending on if the time quantum has been spent.

the context switching logic should be similar to PSJF, but at the end our MLFQ doesn't seems to be running very well. The testing below are done in PSJF.


## Metrics

./external_cal 1
***************************
Total run time: 503 micro-seconds
Total sum is: -649892461
Total context switches 3 
Average turnaround time 503.827769 
Average response time  503.850939 
***************************
./external_cal 6
***************************
Total run time: 514 micro-seconds
Total sum is: -649892461
Total context switches 13 
Average turnaround time 513.276029 
Average response time  513.287053 
***************************
./external_cal 10
***************************
Total run time: 483 micro-seconds
Total sum is: -649892461
Total context switches 21 
Average turnaround time 482.825235 
Average response time  482.896823 
***************************
./external_cal 50
***************************
Total run time: 505 micro-seconds
Total sum is: -649892461
Total context switches 101 
Average turnaround time 504.268504 
Average response time  504.540801 
***************************

ryd4@cpp:~/cs416/threadlib/code/benchmarks$ ./parallel_cal 1
***************************
Total run time: 1981 micro-seconds
verified sum is: 83842816
Total sum is: 83842816
Total context switches 3 
Average turnaround time 1980.260892 
Average response time  1980.265516 
***************************
ryd4@cpp:~/cs416/threadlib/code/benchmarks$ ./parallel_cal 6
***************************
Total run time: 1978 micro-seconds
verified sum is: 83842816
Total sum is: 83842816
Total context switches 13 
Average turnaround time 1977.522690 
Average response time  1977.531597 
***************************
ryd4@cpp:~/cs416/threadlib/code/benchmarks$ ./parallel_cal 10
***************************
Total run time: 1980 micro-seconds
verified sum is: 83842816
Total sum is: 83842816
Total context switches 21 
Average turnaround time 1979.362688 
Average response time  1979.379826 
***************************
ryd4@cpp:~/cs416/threadlib/code/benchmarks$ ./parallel_cal 50
***************************
Total run time: 1980 micro-seconds
verified sum is: 83842816
Total sum is: 83842816
Total context switches 101 
Average turnaround time 1979.258650 
Average response time  1979.325612 
***************************

./vector_multiply 1
***************************
Total run time: 38 micro-seconds
verified sum is: 631560480
Total sum is: 631560480
Total context switches 3 
Average turnaround time 38.168003 
Average response time  38.173617 
***************************
 ./vector_multiply 6
***************************
Total run time: 41 micro-seconds
verified sum is: 631560480
Total sum is: 631560480
Total context switches 13 
Average turnaround time 41.081185 
Average response time  41.094704 
***************************
./vector_multiply 10
***************************
Total run time: 41 micro-seconds
verified sum is: 631560480
Total sum is: 631560480
Total context switches 21 
Average turnaround time 41.751925 
Average response time  41.774248 
***************************
./vector_multiply 50
***************************
Total run time: 61 micro-seconds
verified sum is: 631560480
Total sum is: 631560480
Total context switches 101 
Average turnaround time 61.200254 
Average response time  61.284306 
***************************

On average, our threading library is a few times slower than the pthread library implementation

take ./parrllel_cal 6 as an example, our library used 1978 micro-seconds to complete, when it took 387 micro seconds for p-thread

This comparison is also present in other test, proving the pthread library's superiority over our implementation.