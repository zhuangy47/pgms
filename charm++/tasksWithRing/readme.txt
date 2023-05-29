A program to illustrate use of tasks in scenario similar to that mentioned by Nils Deppe (slack)

Do not use with +p1 (unless you want to help fix the issue).
The "expedited" on forwardRing EP is necessary. 
See comments at the end

Here, a chare array is sending a single message around, while concurrently executing a bunch of tasks.
We may want to do a few rounds before the tasks start to get the time per round without interference from tasks.
Then each chare fires K tasks, for calculating first K Fibonacci (Same as Pingala, who discovered them centuries earlier. Just a fun fact)
numbers. The calculation si done most inefficiently (since that is not the point) by firing chares recursively.
Each task is therefore extremely tiny here. We will use steal queu (TaskQ in Charm.. the build must be SMP and done with --enable-task-queue)

Since the Charm scheduler pays attention to all other messages first, and only looks up taskqueu when its idle,
the chare array ring should have very little interference.. a couple of tasks at most while the ring message awaits.
That is what we want to observe here.

Tangential musing:
Incidentally, the usual practice of sending a message to oneself to allow other activities to proceed (which we are not using here) will interfere with "on idle" logic of task queue. May need to steal even when regular messages are available, with some frequency. Or create an even lower priority queue for such send-to-self "run after everything" messages.

In addition to illustrating how to express such  tasks, it also exposes need for better control
of queuing strategies (+p1 issue), to be fixed orthogonally in the scheduler. +p1 problem, imo,
is because the scheduler selecting local messages over taskQ, completely starving taskq.
(Need to confirm by looking at the scheduler code).
Not a problem for most real codes.

