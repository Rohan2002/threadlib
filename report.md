# CS416 Project 2: User-level Thread Library and Scheduler

Rohan Deshpande (ryd4)
Jinyue (Eric) Liu (jl2661)

## Compile Steps

Run ```make``` to compile binaries. Binaries are stored in the bin/ folder.
Run ```make clean``` to clean build. 

## Part 1. Thread Library

### Thread creation 

### Thread yield

### Thread join

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

### MLFQ Scheduler


## Metrics