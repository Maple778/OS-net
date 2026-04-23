#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

// Message structure
struct msgbuf {
    long mtype;
    char mtext[12];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s message-queue-id num-nodes\n", argv[0]);
        exit(1);
    }

    key_t key = atoi(argv[1]);
    int num_nodes = atoi(argv[2]);
    int msqid = msgget(key, 0);
    if (msqid == -1) {
        perror("msgget hacker");
        exit(1);
    }

    struct msgbuf msg;
    uint32_t *message_info;
    size_t msgsz = sizeof(msg.mtext);

    printf("Smart Hacker started, targeting %d-node system\n", num_nodes);
    printf("Attempting to impersonate legitimate nodes...\n");
    fflush(stdout);

    srand(time(NULL));

    while (1) {
        // Choose a REAL node to impersonate (1 to num_nodes)
        int fake_node = (rand() % num_nodes) + 1;
        int target_node = (rand() % num_nodes) + 1;
        int fake_request = rand() % 10000;  // High request number to get priority

        // Send fake REQUEST message
        msg.mtype = target_node;  // Target a real node
        message_info = (uint32_t *)msg.mtext;
        message_info[0] = 0;  // REQUEST
        message_info[1] = fake_node;  // Fake sender (but valid node ID)
        message_info[2] = fake_request;  // High fake request number

        if (msgsnd(msqid, &msg, msgsz, 0) == -1) {
            perror("msgsnd hacker");
        } else {
            printf("Smart Hacker: Impersonating node %d, sending REQUEST with req_num %d to node %d\n",
                   fake_node, fake_request, target_node);
            fflush(stdout);
        }

        // Also try to send fake REPLY messages
        msg.mtype = num_nodes + target_node;
        message_info[0] = 1;  // REPLY
        message_info[1] = fake_node;
        message_info[2] = 0;

        if (msgsnd(msqid, &msg, msgsz, 0) == -1) {
            // Silently fail
        }

        // Sleep for random time
        sleep(rand() % 3 + 1);
    }

    return 0;
}