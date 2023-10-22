#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define STACK_SIZE SIGSTKSZ

void my_function(int arg1, int arg2) {
    printf("Function called with args: %d, %d\n", arg1, arg2);
}

void entry_point() {
    // Retrieve the arguments from a closure and call the target function.
    int arg1 = 10;
    int arg2 = 20;
    my_function(arg1, arg2);
}

int main() {
    ucontext_t context;

    int res = getcontext(&context);
    if (res < 0){
        fprintf(stdout, "error getcontext\n");
        exit(EXIT_FAILURE);
    }
    // Allocate and set a stack for the context
    void *stack = malloc(STACK_SIZE);  // Allocate a stack
    context.uc_stack.ss_sp = stack;
    context.uc_stack.ss_size = STACK_SIZE;
    context.uc_stack.ss_flags = 0;
    //context.uc_link = NULL;

    // Set the entry point for the context
    makecontext(&context, entry_point, 0);

    // Switch to the new context
    setcontext(&context);

    // Remember to free the stack when done
    free(stack);

    return 0;
}
