#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 2024

static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

void* init_matrix_a(void* arg) { 
    int i = (int)(long)arg;
    for(int j = 0; j < SIZE; j++){
        a[i][j] = 1.0;
    }
    return NULL;
}

void* init_matrix_b(void* arg) { 
    int i = (int)(long)arg;
    for(int j = 0; j < SIZE; j++){
        b[i][j] = 1.0;
    }
    return NULL;
}

void* matmul_seq(void* arg) { 
    int i = (int)(long)arg;   
    for (int j = 0; j < SIZE; j++) {
        c[i][j] = 0.0;
        for (int k = 0; k < SIZE; k++)
            c[i][j] += a[i][k] * b[k][j];
    }
    return NULL;
}

void* match_create_and_join(void* (*func) (void*)){
    pthread_t threads[SIZE];
    // A new thread shall be created for each row to be calculated in the matrix.
    for (int i = 0; i < SIZE; i++) { 
        pthread_create(&threads[i], NULL, func, (void*)(long)i); 
    }
    //The main function shall wait until all threads have terminated before the program terminates.
    for (int i = 0; i < SIZE; i++) {
        pthread_join(threads[i], NULL); 
    }
    return NULL;
}

int main() {

    match_create_and_join(init_matrix_a); //Use one thread to initialize each of the rows in the matrices a and b
    match_create_and_join(init_matrix_b); //Use one thread to initialize each of the rows in the matrices a and b
    match_create_and_join(matmul_seq);

    return 0;
}

/*
How long was the execution time of your parallel program?
Which speedup did you obtain compared to the sequential version (Speedup = Tsequential / Tparallel)?
I repeated the measurements several times, and my results for SIZE = 1024 were as follows:
| Times | task13 speedup        | task14 speedup        |
| --- | --------------------- | --------------------- |
| 1   | 3.498 / 1.525 ≈ 2.295 | 3.498 / 1.605 ≈ 2.179 |
| 2   | 3.498 / 1.581 ≈ 2.212 | 3.498 / 1.572 ≈ 2.226 |
| 3   | 3.498 / 1.431 ≈ 2.445 | 3.498 / 1.418 ≈ 2.467 |
| 4   | 3.498 / 1.487 ≈ 2.352 | 3.498 / 1.572 ≈ 2.226 |
| 5   | 3.498 / 1.459 ≈ 2.397 | 3.498 / 1.632 ≈ 2.143 |
| 6   | 3.498 / 1.423 ≈ 2.459 | 3.498 / 1.606 ≈ 2.178 |
| 7   | 3.498 / 1.455 ≈ 2.404 | 3.498 / 1.551 ≈ 2.255 |

But wheLater, I increased SIZE to 2024, and the behavior changed significantly:
| Times | task13 speedup         | task14 speedup         |
| ----- | ---------------------- | ---------------------- |
| 1     | 59.505 / 18.219 ≈ 3.26 | 59.505 / 14.442 ≈ 4.12 |
| 2     | 59.505 / 18.419 ≈ 3.23 | 59.505 / 13.577 ≈ 4.38 |

When SIZE = 1024, both task13 (with bonly matrix multiplication are parallelized) 
and task14 (with both matrix initialization and matrix multiplication are parallelized) give similar speedups. 
Sometimes task13 is more quicly, sometimes is task14.
This happens because, at this matrix size, the cost of initializing matrix a and b is relatively small 
compared to the cost of the matrix multiplication itself. Even though task14 parallelizes 
the initialization of both matrices, the time saved is small, and the overhead of creating additional 
threads largely cancels out the benefit.

However, when SIZE increases to 2024, the situation changes dramatically:
The total amount of work for matrix initialization grows quadratically.
The work for matrix multiplication grows even faster.
Creating one thread per row becomes much more beneficial.
In task14, both matrix initialization and matrix multiplication are parallelized.
Task13 only parallelizes the multiplication step and keeps initialization sequential.
Therefore, for larger matrices, task14 gains a significant advantage because it uses 
parallelism for a larger portion of the total work. 
This results in noticeably better speedup.

In summary:
For small matrix sizes, parallelizing the initialization does not help much 
because the overhead of creating threads dominates the benefit.
For larger matrix sizes, the initialization becomes expensive enough that 
parallelizing it (task14) leads to substantial performance improvement.
*/ 