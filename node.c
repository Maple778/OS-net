#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#define MAX_NODES 100
#define REPLY 0

// Message structure for node communication
struct msgbuf {
    long mtype;
    char mtext[12];
};

// Message structure for printer communication
struct printer_msgbuf {
    long mtype;
    char mtext[1024];
};

// Shared memory structure
struct shared_vars {
    int request_number;          /* node's sequence number */
    int highest_request_number;  /* highest request number seen */
    int outstanding_reply;       /* # of outstanding replies */
    int request_CS;              /* true when node requests critical section */
    int in_CS;                   /* true when node is inside critical section */
    int reply_deferred[MAX_NODES]; /* reply_deferred[i] is true when node defers reply to node i */
    int message_count;           /* number of messages sent */
};

// Global variables (not shared)
int me;                      /* my node number */
int N;                       /* number of nodes */
int msqid;                   /* message queue ID */
int start_wait_time;         /* starting wait time */
int repeat_wait_time;        /* repeat wait time */

// Pointer to shared memory
struct shared_vars *shared;

// Semaphores
int mutex_sem;              /* for mutual exclusion to shared variables */
int wait_sem;                /* used to wait for all requests */

// Semaphore operations
void P(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void V(int semid) {
    struct sembuf op = {0, 1, 0};
    semop(semid, &op, 1);
}

// Function prototypes
void send_message(int to, int flag, int req_num);
void send_to_printer(const char *message);
void receive_request(int sender, int req_num);
void receive_reply();
void enter_critical_section();
void handle_requests();
void handle_replies();
void P(int semid);
void V(int semid);

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s node-id node-num message-queue-id start-wait-time repeat-wait-time\n", argv[0]);
        exit(1);
    }

    // Parse command line arguments
    me = atoi(argv[1]);
    N = atoi(argv[2]);
    msqid = atoi(argv[3]);

    start_wait_time = atoi(argv[4]);
    repeat_wait_time = atoi(argv[5]);

    // Create shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct shared_vars), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory
    shared = (struct shared_vars *)shmat(shmid, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Create semaphores
    mutex_sem = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    if (mutex_sem == -1) {
        perror("semget mutex");
        exit(1);
    }
    semctl(mutex_sem, 0, SETVAL, 1);  // Initialize to 1

    wait_sem = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    if (wait_sem == -1) {
        perror("semget wait");
        exit(1);
    }
    semctl(wait_sem, 0, SETVAL, 0);  // Initialize to 0

    // Initialize shared variables
    shared->request_number = 0;
    shared->highest_request_number = 0;
    shared->outstanding_reply = 0;
    shared->request_CS = 0;
    shared->in_CS = 0;
    shared->message_count = 0;

    for (int i = 0; i < MAX_NODES; i++) {
        shared->reply_deferred[i] = 0;
    }

    // Seed random number generator
    srand(time(NULL) + me);

    // Fork to create three processes
    pid_t pid_request, pid_reply;

    pid_request = fork();
    if (pid_request == 0) {
        // Child process 1: Handle incoming REQUEST messages
        handle_requests();
        exit(0);
    }

    pid_reply = fork();
    if (pid_reply == 0) {
        // Child process 2: Handle incoming REPLY messages
        handle_replies();
        exit(0);
    }

    // Parent process: Mutual exclusion process
    enter_critical_section();

    // Wait for child processes to finish
    waitpid(pid_request, NULL, 0);
    waitpid(pid_reply, NULL, 0);

    // Detach and remove shared memory
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);

    // Remove semaphores
    semctl(mutex_sem, 0, IPC_RMID, 0);
    semctl(wait_sem, 0, IPC_RMID, 0);

    return 0;
}

void send_message(int to, int flag, int req_num) {
    struct msgbuf msg;
    uint32_t *message_info = (uint32_t *)msg.mtext;

    // Set message type
    if (flag == 1) {
        // REPLY message
        msg.mtype = N + to;
    } else {
        // REQUEST message
        msg.mtype = to;
    }

    // Set message content
    message_info[0] = flag;        // 0 for request, 1 for response
    message_info[1] = me;          // Sender node id
    message_info[2] = req_num;     // Request Number

    // Send message
    if (msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    shared->message_count++;
}

void send_to_printer(const char *message) {
    struct printer_msgbuf printer_msg;
    size_t msg_len;

    // Set message type for printer: 2 * num_nodes + 1
    printer_msg.mtype = 2 * N + 1;

    // Clear buffer
    memset(printer_msg.mtext, 0, sizeof(printer_msg.mtext));

    // Copy message to mtext
    strncpy(printer_msg.mtext, message, sizeof(printer_msg.mtext) - 1);

    // Get message length (include null terminator for safety)
    msg_len = strlen(printer_msg.mtext);

    // Send to printer (use actual message length)
    if (msgsnd(msqid, &printer_msg, msg_len, 0) == -1) {
        // Silent fail - printer might not be running
    }
}

void receive_request(int sender, int req_num) {
    int defer_it;  /* true if request must be deferred */

    // Update highest request number
    if (req_num > shared->highest_request_number) {
        shared->highest_request_number = req_num;
    }

    P(mutex_sem);

    // Agerwala's three-state logic:
    // 1. In critical section: always defer
    // 2. Waiting for replies: defer only if we have priority
    // 3. Idle: never defer
    if (shared->in_CS) {
        defer_it = 1;
    } else if (shared->request_CS) {
        defer_it = (req_num > shared->request_number) ||
                   (req_num == shared->request_number && sender > me);
    } else {
        defer_it = 0;
    }

    V(mutex_sem);

    /* defer_it is true if we should defer the reply */
    if (defer_it) {
        P(mutex_sem);
            shared->reply_deferred[sender] = 1;
        V(mutex_sem);
    } else {
        send_message(sender, 1, 0);
    }
}

void receive_reply() {
    P(mutex_sem);
        shared->outstanding_reply--;
    V(mutex_sem);
    V(wait_sem);
}

void enter_critical_section() {
    int iteration = 0;

    // Wait for start time
    sleep(start_wait_time);

    while (1) {
        iteration++;

        // Send REQUEST to all nodes
        P(mutex_sem);
            shared->request_CS = 1;
            shared->request_number = shared->highest_request_number + 1;
            shared->highest_request_number = shared->request_number;
            shared->outstanding_reply = N - 1;
        V(mutex_sem);

        for (int i = 1; i <= N; i++) {
            if (i != me) {
                send_message(i, 0, shared->request_number);
            }
        }

        // Wait for all replies using semaphore
        while (shared->outstanding_reply != 0) {
            P(wait_sem);
        }

        // Enter critical section
        P(mutex_sem);
            shared->in_CS = 1;
        V(mutex_sem);

        // Format and send complete message
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[9];
        char buffer[1024];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

        sprintf(buffer, "************ CLIENT %d START ************\nCLIENT %d MESSAGE NUMBER %d: Geng Li %s\n************ CLIENT %d END **************\n",
                me, me, shared->message_count, time_str, me);
        send_to_printer(buffer);

        // Exit critical section
        P(mutex_sem);
            shared->in_CS = 0;
            shared->request_CS = 0;
        V(mutex_sem);

        // Send deferred replies
        for (int i = 1; i <= N; i++) {
            P(mutex_sem);
                if (shared->reply_deferred[i]) {
                    shared->reply_deferred[i] = 0;
                    V(mutex_sem);
                    send_message(i, 1, 0);
                } else {
                    V(mutex_sem);
                }
        }

        // Wait for random time before next request
        if (repeat_wait_time > 0) {
            int wait_time = rand() % (repeat_wait_time + 1);
            sleep(wait_time);
        } else {
            break;
        }
    }
}

void handle_requests() {
    struct msgbuf msg;
    uint32_t *message_info;
    size_t msgsz = sizeof(msg.mtext);

    while (1) {
        // Receive REQUEST message (mtype = me)
        // Use IPC_NOWAIT to check if queue is still valid
        ssize_t result = msgrcv(msqid, &msg, msgsz, me, 0);

        if (result == -1) {
            // Queue was removed or error occurred
            exit(0);
        }

        message_info = (uint32_t *)msg.mtext;

        // Check if it's a REQUEST message (flag = 0)
        if (message_info[0] == 0) {
            int sender = message_info[1];
            int req_num = message_info[2];
            receive_request(sender, req_num);
        }
    }
}

void handle_replies() {
    struct msgbuf msg;
    uint32_t *message_info;
    size_t msgsz = sizeof(msg.mtext);
    long reply_type = N + me;

    while (1) {
        // Receive REPLY message (mtype = N + me)
        ssize_t result = msgrcv(msqid, &msg, msgsz, reply_type, 0);

        if (result == -1) {
            // Queue was removed or error occurred
            exit(0);
        }

        message_info = (uint32_t *)msg.mtext;

        // Check if it's a REPLY message (flag = 1)
        if (message_info[0] == 1) {
            receive_reply();
        }
    }
}
