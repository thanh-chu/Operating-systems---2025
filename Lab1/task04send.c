#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PERMS 0644
struct my_msgbuf {
    long mtype;
	int number;
    char mtext[200];
};

int main(void) {
    struct my_msgbuf buf;
    int msqid;
    int len;
    key_t key;
    system("touch msgq.txt");

    if ((key = ftok("msgq.txt", 'B')) == -1) {
        perror("ftok");
        exit(1);
    }

    if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }

    printf("message queue: ready to send messages.\n");
    printf("Tryck Enter för att starta skickning...\n");
    buf.mtype = 1; /* we don't really care in this case */

    if ((fgets(buf.mtext, sizeof buf.mtext, stdin) != NULL) ){

        len = sizeof(buf.number);

        for(int i = 0; i < 50; i ++){
            buf.number = rand() % 101;

         if (msgsnd(msqid, &buf, len, 0) == -1) /* +1 for '\0' */
            perror("msgsnd");
        }
    }

    buf.number = -1;
    if (msgsnd(msqid, &buf, len, 0) == -1) /* +1 for '\0' */
        perror("msgsnd");

    // sleep(5);

    // if (msgctl(msqid, IPC_RMID, NULL) == -1) {
    //    perror("msgctl");
    //    exit(1);
    // }
    printf("message queue: done sending messages.\n");
    return 0;
}


