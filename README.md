# network-algo-requirements
Unified interface definition for network mutual exclusion algorithm

**SUBMISSION DEADLINE APRIL 25**

## Command Line Interface
- For final integration, we must be able to run programs through a command line interface so we can spawn them and let them work simultaneously.
- Command line arguments include the node id, number of nodes, the message queue id, the starting wait time, and the repeat wait time. All arguments are guaranteed to be positive integers.
  - The node id is the id of the node that your program will be.
  - The number of nodes is the total number of requesting nodes in the system (including yours); you can assume that node IDs are integers evenly spaced in the range `[1, node-num]`.
  - The message queue id is the id of the message queue to send messages on.
  - The starting wait time is the minimum number of seconds to wait before sending the first request.
  - The repeat wait time is the maximum number of seconds to wait after a request before trying to send another (random from 0 to this time). You should be able to respond to requests during this time.
- Programs should be compatible with and runnable using the following command line interface
```
program node-id node-num message-queue-id start-wait-time repeat-wait-time
```
For example:
```
./myprogram 3 4 1234 3 0
```

For final usage, all the programs will be spawned in a form like this (you aren't implementing this):
```c
sprintf(cmd, "yourprogram %d %d %d %d %d", node_id, node_num, msqid, wait_time, repeat_wait);
system(cmd);
```

Your programs should print out this format:
```
************ CLIENT node-id START ************
CLIENT node-id MESSAGE NUMBER messages-sent: Your Name Time in hh:mm:ss
************ CLIENT node-id END **************
```
For instance,
```
************ CLIENT 2 START ************
CLIENT 2 MESSAGE NUMBER 24: Boone Pool 12:32:53
************ CLIENT 2 END **************
```

## Message Format
- For more info on using message queues, see [here](https://man7.org/linux/man-pages/man2/msgsnd.2.html)
- Programs must follow same msgbuf format
- Message type argument should be `recipient_node_id` for all request messages messages. That is, the `long mtype` should be the node id of the intended recipient of your message.
- For all response messages, your message type should be `node_num + recipient_node_id`.
- `msgbuf` should have `char mtext[12]` which we can treat like an integer array for requests.
```c
message_info = (uint32_t *) mtext;
message_info[0]; // Flag: 0 for request, 1 for response
message_info[1]; // Sender node id
message_info[2]; // Request Number
```

Message format for printer should have message type `2 * num_nodes + 1` and a `char *mtext` that contains the message including newlines if necessary.

## Printer and Hacker Interface
The printer should have the following command line interface:
```
printer id message-queue-id
```
And should receive messages on message type `2 * num_nodes + 1`.

The hacker should have the following command line interface:
```
hacker message-queue-id
```

