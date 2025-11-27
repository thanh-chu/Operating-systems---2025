#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid1, pid2;
    unsigned i;
    unsigned niterations = 100;

    pid1 = fork();

    if (pid1 == 0) {
        // Child #1
        for (i = 0; i < niterations; ++i) {
            printf("A = %u, ", i);
        }
        printf("\n");
    } else {
        // create Child 2
        pid2 = fork();
        if (pid2 == 0) {
            // Child 2 prints C
            for (i = 0; i < niterations; ++i) {
                printf("C = %u, ", i);
            }
            printf("\n");
        } else {
            // prints B
            for (i = 0; i < niterations; ++i) {
                printf("B = %u, ", i);
            }
            printf("\n");

            printf("In parent => child #1 pid: %d\n", pid1);
            printf("In parent => child #2 pid: %d\n", pid2);

            wait(NULL);
            wait(NULL);
        }
    }

    return 0;
}