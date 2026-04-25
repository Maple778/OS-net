#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MAX_NODES 100

// Message structure
struct msgbuf {
    long mtype;
    char mtext[12];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s num-nodes message-queue-id\n", argv[0]);
        exit(1);
    }

    int num_nodes = atoi(argv[1]);
    int msqid = atoi(argv[2]);

    struct msgbuf msg;
    uint32_t *message_info;
    size_t msgsz = sizeof(msg.mtext);

    printf("Hacker started, monitoring message queue %d\n", msqid);
    fflush(stdout);

    srand(time(NULL));

    while (1) {
        // Randomly choose a node to impersonate (valid range)
        int fake_node = (rand() % num_nodes) + 1;
        int target_node = (rand() % num_nodes) + 1;
        int fake_request = rand() % 1000;

        // Send fake REQUEST message
        msg.mtype = target_node;  // Target a valid node
        message_info = (uint32_t *)msg.mtext;
        message_info[0] = 0;  // REQUEST
        message_info[1] = fake_node;  // Fake sender
        message_info[2] = fake_request;  // Fake request number

        if (msgsnd(msqid, &msg, msgsz, 0) == -1) {
            perror("msgsnd hacker");
        } else {
            printf("Hacker: Sent fake REQUEST from node %d with req_num %d to node %ld\n",
                   fake_node, fake_request, msg.mtype);
            fflush(stdout);
        }

        // Sleep for random time
        sleep(rand() % 5 + 1);
    }

    return 0;
}
