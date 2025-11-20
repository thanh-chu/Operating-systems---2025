/***************************************************************************
 *
 * Sequential version of Matrix-Matrix multiplication
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define SIZE 1024

static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

struct threadArgs {
	unsigned int id;
};

static void
init_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++) {
            /* Simple initialization, which enables us to easy check
            * the correct answer. Each element in c will have the same
            * value as SIZE after the matmul operation.
            */
            a[i][j] = 1.0;
            b[i][j] = 1.0;
        }
}

void* parrallel(void* params){
    struct threadArgs *args = (struct threadArgs*) params;
    unsigned int i = args->id;

    int j, k;
    for (j = 0; j < SIZE; j++) {
        c[i][j] = 0.0;
        for (k = 0; k < SIZE; k++)
            c[i][j] = c[i][j] + a[i][k] * b[k][j];
    }
    return NULL;
}

static void
matmul_seq()
{
    // int i;

    // for (i = 0; i < SIZE; i++) {
    //     parrallel(i);
    // }
    pthread_t* children;
    struct threadArgs* args; // argument buffer

    children = malloc(SIZE * sizeof(pthread_t)); // allocate array of handles
	args = malloc(SIZE * sizeof(struct threadArgs)); // args vector

	for (unsigned int id = 0; id < SIZE; id++) {
		// create threads
		args[id].id = id;

		pthread_create(&(children[id]), // our handle for the child
			NULL, // attributes of the child
			parrallel, // the function it should run
			(void*)&args[id]); // args to that function
	}

    printf("I am the parent (main) thread.\n");

	for (unsigned int id = 0; id < SIZE; id++) {
		pthread_join(children[id], NULL );
	}
}


static void
print_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++)
            printf(" %7.2f", c[i][j]);
        printf("\n");
    }
}

static double get_time_ms(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

int
main(int argc, char **argv)
{
    init_matrix();
    double t0 = get_time_ms();
    matmul_seq();
    //print_matrix();

    double t1 = get_time_ms();

    printf("threaded matmul took %.3f ms (%.3f s)\n", t1 - t0, (t1 - t0) / 1000.0);

    // Kontroll (alla element ska bli SIZE, t.ex. c[0][0])
    printf("c[0][0] = %.2f (expected %.2f)\n", c[0][0], (double)SIZE);
}