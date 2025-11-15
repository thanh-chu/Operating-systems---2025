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
};

int main(void) {
    struct my_msgbuf buf;
    int msqid;
    key_t key;

    if ((key = ftok("msgq.txt", 'B')) == -1) {
        perror("ftok");
        exit(1);
    }

    if ((msqid = msgget(key, PERMS)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }

    printf("message queue: ready to receive integers.\n");

    while (1) {
        if (msgrcv(msqid, &buf, sizeof(buf.number), 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        if (buf.number == -1) { // -1 markerar slutet
            printf("recv: end of transmission (-1)\n");
            break;
        }

        printf("recv: %d\n", buf.number);
    }

    printf("message queue: done receiving integers.\n");
    system("rm msgq.txt");

    return 0;
}

