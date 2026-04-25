#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>

// Message structure for printer
struct msgbuf {
    long mtype;
    char mtext[1024];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s num-nodes message-queue-id\n", argv[0]);
        exit(1);
    }

    int num_nodes = atoi(argv[1]);
    int msqid = atoi(argv[2]);
    long msg_type = 2 * num_nodes + 1;  // Calculate message type for printer
    struct msgbuf msg;
    size_t msgsz;
    msgsz = sizeof(msg.mtext);

    printf("Printer started for %d nodes, listening on message type %ld\n", num_nodes, msg_type);
    fflush(stdout);

    // For flexibility when num_nodes is not specified, try to receive messages
    // If using default type and no messages arrive, the user needs to specify num_nodes
    while (1) {
        // Receive messages for printer
        ssize_t result = msgrcv(msqid, &msg, msgsz, msg_type, 0);

        if (result == -1) {
            if (errno == EINTR) continue;  // Interrupted, try again
            perror("msgrcv printer");
            continue;
        }

        // Ensure message is null-terminated
        if (result < (ssize_t)sizeof(msg.mtext)) {
            msg.mtext[result] = '\0';
        } else {
            msg.mtext[sizeof(msg.mtext) - 1] = '\0';
        }

        // Print the message
        printf("%s", msg.mtext);
        fflush(stdout);
    }

    return 0;
}
