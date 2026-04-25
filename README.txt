CSC 641 Lab
Geng Li

Description: This lab implements the Lamport network mutual exclusion
algorithm, using System V message queues as a proxy for a network and
packets. The implementation includes a distributed mutual exclusion
protocol where nodes communicate via message queues to coordinate access
to a critical section.

Folder Structure:
node.c - Main node implementation with Lamport algorithm
printer.c - Printer process that collects and displays output
hacker.c - Attacker process that sends fake requests to test robustness
create_msq.c - Message queue creation utility
remove_msq.c - Message queue removal utility
Makefile - Build configuration

Instructions:
Build using: make
Clean build files: make clean

Command Line Interface:
node: node-id node-num message-queue-id start-wait-time repeat-wait-time
printer: num-nodes message-queue-id
hacker: num-nodes message-queue-id

Testing:
1. Create message queue:
   ./create_msq 1234
   (Note the returned queue ID, e.g., "Message queue created: 55")

2. Start printer:
   ./printer 3 55 > output.log &
   (Where 3 = number of nodes, 55 = message queue ID from step 1)

3. Start nodes:
   ./node 1 3 55 2 0 &
   ./node 2 3 55 2 0 &
   ./node 3 3 55 2 0 &
   (Format: ./node node-id node-num msqid start-wait repeat-wait)

4. View results:
   cat output.log

5. Test with hacker (optional):
   ./hacker 3 55 &

6. Cleanup:
   killall node printer hacker
   ./remove_msq 1234

