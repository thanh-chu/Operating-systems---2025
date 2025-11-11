#include <stdio.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <stdlib.h> 
#include <time.h>

#define SHMSIZE 128 
#define MY_SHM_R 0400
#define MY_SHM_W 0200
#define BUFFER_SIZE 10 //contains 10 numbers 

int main(int argc, char **argv)
{
	struct shm_struct {
		int buffer[BUFFER_SIZE]; //contains 10 numbers
        int producerIndex;
        int consumerIndex;
		int count;
        int done;
	};
	volatile struct shm_struct *shmp = NULL; 
    char *addr = NULL; 
	pid_t pid = -1;
	int var1 = 0, var2 = 0, shmid = -1; 
	struct shmid_ds *shm_buf;

    srand(time(NULL)); //a random time 

	shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | SHM_R | SHM_W);
   

	shmp = (struct shm_struct *) shmat(shmid, addr, 0);
    shmp->producerIndex = 0;
    shmp->consumerIndex = 0;
    shmp->count = 0;
	pid = fork();

    if(pid != 0){
        while(var1 < 100){
            var1++;
            while(shmp->count == BUFFER_SIZE); //producer wait if bufffer is full
            shmp->buffer[shmp->producerIndex] = var1;
            printf("Producer send: %d (index %d)\n", var1, shmp->producerIndex); 
            fflush(stdout);
            shmp->producerIndex = (shmp->producerIndex+1)%BUFFER_SIZE; //use BUFFER_SIZE to have circular, bounded buffer
            shmp->count++;

            usleep(100000 + rand() % 400000); //sleep 0.1-0.5 seconds
        }
        wait(NULL); 
		shmdt(addr);
		shmctl(shmid, IPC_RMID, shm_buf);
        printf("Producer finished.\n");
	} else {
		while (var2 < 100) {
			while (shmp->count == 0);{ //consumer wait if bufffer is empty
                if (shmp->done)  // nếu producer đã báo hết dữ liệu
                    goto end;
            }
			var2 = shmp->buffer[shmp->consumerIndex];
            printf("Consumer receives: %d (index %d)\n", var2, shmp->consumerIndex); 
            fflush(stdout);

			shmp->consumerIndex = (shmp->consumerIndex+1)%BUFFER_SIZE; //use BUFFER_SIZE to have circular, bounded buffer
            shmp->count--;

			usleep(200000 + rand() % 1800000); //sleep 0.2-2 seconds

		}
		//shmctl(shmid, IPC_RMID, shm_buf);
	}
    end:
        printf("Consumer finished reading all data.\n");
		shmdt(addr);

}
