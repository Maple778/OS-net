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
    if (argc < 3) {
        fprintf(stderr, "Usage: %s id message-queue-id [num_nodes]\n", argv[0]);
        fprintf(stderr, "  num_nodes is optional - if not specified, will attempt to detect from messages\n");
        exit(1);
    }

    int printer_id = atoi(argv[1]);
    key_t key = atoi(argv[2]);
    int msqid = msgget(key, 0);
    if (msqid == -1) {
        perror("msgget printer");
        exit(1);
    }
    int num_nodes = 0;
    long msg_type;
    struct msgbuf msg;
    size_t msgsz;

    // Parse optional num_nodes parameter
    if (argc >= 4) {
        num_nodes = atoi(argv[3]);
        // Calculate message type: 2 * num_nodes + 1
        msg_type = 2 * num_nodes + 1;
        printf("Using specified num_nodes=%d, message type=%ld\n", num_nodes, msg_type);
    } else {
        // If num_nodes not specified, use a default approach
        // The standard message type for printers is typically in a high range
        // We'll start with a reasonable default and can adapt if needed
        msg_type = 201; // Default for 100 nodes: 2*100+1
        printf("No num_nodes specified, using default message type=%ld\n", msg_type);
        printf("For better results, specify num_nodes as 3rd parameter\n");
    }
    msgsz = sizeof(msg.mtext);

    printf("Printer %d started, listening on message type %ld\n", printer_id, msg_type);
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
        if (result < sizeof(msg.mtext)) {
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
