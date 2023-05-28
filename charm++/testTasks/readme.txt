
Program to illustrarte ways of running tasks.
Use only on smp or multicore version.
Also, designed for use with only one logical node. so, do not use charmrun.
3 parameters: baseLoad, maxLoad, tasksPerChare.
run as: hello +p5 10 100 4 , to create 10 chares (2*numpes), each firing 4 chares
with the task times raning from 10 to 100 microseconds

Uses 3 methods of creating tasks:                                                                                                     
   1. do it yourself  (doTasks)                                                                                                  
   2. enqueue them in the node queue (fireNodeTasks), completion callback to nodeTasksCompletged (via SyncCB)                    
   3. enqueue them in the taskq (fireViaTaskQ),  completion callback to taskQTasksCompletged (via SyncCB)                        

With the provided makefile, creates projections trace, including brakceted events for tasks
