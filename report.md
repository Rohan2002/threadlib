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




#### Mutex Initialization & Destroy

#### Lock and Unlock

## Part 2. Scheduler

### PSJF Scheduler

### MLFQ Scheduler


## Metrics