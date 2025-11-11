#include <stdio.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <stdlib.h> 
#include <time.h>
#include <semaphore.h>
#include <fcntl.h> /* For O_* constants */
#include <sys/wait.h>



#define SHMSIZE sizeof(struct shm_struct)
#define BUFFER_SIZE 10 //contains 10 numbers
//#define BUFFER_SIZE 2 //contains 2 numbers


struct shm_struct {
		int buffer[BUFFER_SIZE]; //contains 10 numbers
        int producerIndex;
        int consumerIndex;
        int done;
	};

int main()
{
    int shmid;
    struct shm_struct *shmp;
    pid_t pid;

    sem_t *empty = sem_open("/empty_sem", O_CREAT, 0644, BUFFER_SIZE);
    sem_t *full = sem_open("/full_sem", O_CREAT, 0644, 0);
    sem_t *mutex = sem_open("/mutex_sem", O_CREAT, 0644, 1);

    srand(time(NULL)); //a random time 

    shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | 0666);
    shmp = (struct shm_struct *) shmat(shmid, NULL, 0);

    shmp->producerIndex = 0;
    shmp->consumerIndex = 0;

    pid = fork();

    if(pid > 0){
        for( int i = 1; i <= 100; i++){
            sem_wait(empty); //waiting for an empty cell
            sem_wait(mutex); //in to critical section

            shmp->buffer[shmp->producerIndex] = i;
            printf("Producer send: %d (index %d)\n", i, shmp->producerIndex); 
            fflush(stdout);
            shmp->producerIndex = (shmp->producerIndex+1)%BUFFER_SIZE; //use BUFFER_SIZE to have circular, bounded buffer

            sem_post(mutex); //leave from critical section
            sem_post(full); //report another filled cell
            //usleep(0);
            usleep(100000 + rand() % 400000); //sleep 0.1-0.5 seconds
        }
        wait(NULL); //wait for child process
        shmdt(shmp);
        shmctl(shmid, IPC_RMID, NULL); //delete shared memory

        sem_close(empty);
        sem_close(full);
        sem_close(mutex);

        sem_unlink("/empty_sem");
        sem_unlink("/full_sem");
        sem_unlink("/mutex_sem");
        
        printf("Producer finished.\n");
    } else {
        int received[100] = {0};
        for(int i = 1; i <= 100; i++){
            sem_wait(full); //wating for data
            sem_wait(mutex); //in to cirtical section

            int val = shmp->buffer[shmp->consumerIndex];
            printf("Consumer receiver: %d (index %d)\n", val, shmp->consumerIndex); 
            fflush(stdout);
            shmp->consumerIndex = (shmp->consumerIndex+1)%BUFFER_SIZE;

            sem_post(mutex); //leave from critical section
            sem_post(empty); //report another empty cell
            if (val < 1 || val > 100) { //only for make sure that semathores works well
                printf("Error: value out of range: %d\n", val);
            } else if (received[val-1] == 1) {
                printf("Error: duplicate value received: %d\n", val);
            } else {
                received[val-1] = 1; // đánh dấu đã nhận
            }
            //usleep(0);
            usleep(200000 + rand() % 1800000); //sleep 0.2-2 seconds

        }
        shmdt(shmp); //detach memory area when fishes (not use it any more at this time)

        sem_close(empty);
        sem_close(full);
        sem_close(mutex);
        printf("Consumer finished reading all data.\n");
        for (int i = 0; i < 100; i++) {
        if (received[i] == 0) {
            printf("Missing value: %d\n", i+1);
        }
        }
    }
}
/*
Why did the problems in Task 2 occur, and how does your solution with semaphores solve them?
The problems in Task 2 occur is that:
- Both producer and consumer access `count` and indices without mutual exclusion
=> Producer reads count, sees it is not full.
At the same time, consumer reads count, sees it is not empty.
Both update the buffer/index simultaneously → data could be overwritten 
or read multiple times.
- Using "busy waiting" => waste CPY cycles, especially if the buffer is empty or full for long periods

How does your solution with semaphores solve them?
Haing 3 semaphores:
| Semaphore | Purpose                                         | Initial value |
| --------- | ----------------------------------------------- | ------------- |
| `empty`   | Counts empty slots in the buffer                | `BUFFER_SIZE` |
| `full`    | Counts full slots in the buffer                 | 0             |
| `mutex`   | Protects the critical section (buffer, indices) | 1             |

sem_wait(mutex) and sem_post(mutex): make sure that only one process at a time can access the buffer and indices

*/
