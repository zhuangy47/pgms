A program to illustrate use of tasks in scenario similar to that mentioned by Nils Deppe (slack)

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
