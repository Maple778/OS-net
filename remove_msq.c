#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <message_queue_id>\n", argv[0]);
        return 1;
    }

    int msqid = atoi(argv[1]);
    int queue_id = msgget(msqid, 0666);

    if (queue_id == -1) {
        perror("msgget");
        return 1;
    }

    if (msgctl(queue_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        return 1;
    }

    printf("Message queue removed: %d\n", queue_id);
    return 0;
}
