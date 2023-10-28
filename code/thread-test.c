#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "thread-worker.h"

#define STACK_SIZE SIGSTKSZ

void my_function(int arg1, int arg2) {
    printf("Function called with args: %d, %d\n", arg1, arg2);
}

int x = 0;
int loop = 10000000;

worker_mutex_t thread_mutex;
void *add_counter(void *arg)
{
    int *p = malloc(sizeof(int));
    *p = 5;
    int i;
    /* Add thread synchronizaiton logic in this function */
    // Locks the resource with the established mutex, no other thread may access it before it is unlocked.
    // worker_mutex_lock(&thread_mutex);
    for (i = 0; i < loop; i++)
    {
        // Modify the resource while x is mutex locked
        x = x + 1;
        printf("Thread id: %d modified X=%d\n", getCurrentThread()->thread_id, x);
    }
    // unlock the mutex, allowing other threads to access x
    // worker_mutex_unlock(&thread_mutex);

    return p;
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

void entry_point() {
    // Retrieve the arguments from a closure and call the target function.
    int arg1 = 10;
    int arg2 = 20;
    my_function(arg1, arg2);
}
int main(int argc, char **argv)
{
    int thread_num = 5;
    int i = 0;

    int mutex_init = worker_mutex_init(&thread_mutex, NULL);
    if (!mutex_init)
    {
        perror("Failed to create mutex\n");
    }

    worker_t *thread = (worker_t *)malloc(thread_num * sizeof(worker_t));

    for (i = 0; i < thread_num; ++i)
    {
        thread[i] = i;
        int tid = worker_create(&thread[i], NULL, add_counter, NULL);
        printf("Created thread id: %d\n", tid);
    }
    // printf("Current thread id: %d\n", getCurrent)
    // worker_create(&thread[0], NULL, add_counter, NULL);
    // worker_create(&thread[1], NULL, add_counter, NULL);

    for (i = 0; i < thread_num; ++i)
    {
        worker_join(thread[i], NULL);
    }

    // worker_t *tid_pointer2 = malloc(sizeof(worker_t));
    // *tid_pointer2 = 2;
    // worker_t *tid_pointer3 = malloc(sizeof(worker_t));
    // *tid_pointer3 = 3;

    // int *i1 = malloc(sizeof(int));
    // int *i2 = malloc(sizeof(int));
    // *i1 = 4;
    // *i2 = 5;
    // int tid0 = worker_create(tid_pointer2, NULL, add_counter, NULL);
    // int tid1 = worker_create(tid_pointer3, NULL, add_counter, NULL);

    // // int tid0 = worker_create(tid_pointer2, NULL, inf_loop, i1);
    // // int tid1 = worker_create(tid_pointer3, NULL, inf_loop, i2);

    // printf("Created tid: %d\n", tid0);
    // printf("Created tid: %d\n", tid1);

    // // void *tid0_ret = malloc(sizeof(int));
    // // void *tid1_ret = malloc(sizeof(int));

    // worker_join(tid0, NULL);
    // worker_join(tid1, NULL);

    // printf("Main thread tid0 ret val: %d\n", *(int *)tid0_ret);
    // printf("Main thread tid0 ret val: %d\n", *(int *)tid1_ret);
    // printf("The thread X value: %d\n", x);

    // // while (1);
    // free(args);
    // free(tid_pointer2);
    // free(tid_pointer3);

    // printf("Reached this control block\n");
    // return EXIT_SUCCESS;

    // // simulate timer.
    // // create_thread_timer();
    // // while(1);
}