#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <message_queue_id>\n", argv[0]);
        return 1;
    }

    key_t key = atoi(argv[1]);
    int queue_id = msgget(key, 0666 | IPC_CREAT);

    if (queue_id == -1) {
        perror("msgget");
        return 1;
    }

    printf("Message queue created: %d (key: %d)\n", queue_id, key);
    return 0;
}
