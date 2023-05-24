This simple program tests and illustrates the TRAM library for fixed size messages.

It should be used with at least (but may be exactly) 2 PEs. Any more than 2 will be unused.
PE 0 sends a large number (totalSends, read as 1st parameter from the command line) messages of 1 integer payload to "ping" method on PE 1.
PE 1 waits for receipt of totalSends messages, and then responds with a single ack message, receiving which, pe 0 calls ckexit.

"aggregate" keyword is used on the ping entry method (which can be omited for performance comparisons).
Every sendBatch messages, the sender thread yields to allow progress of the library. (here, it may be unnecessary, but its good
to have it for better illustration via projections.

I used totalSends = 20,000 and sendBatch = 500 . The buffer size is 1900 (specified in the .ci file.. which is a pity, since I cannot vary it without recompilation).
You can try with other sizes.

3 runs I (Sanjay) did were:
  1. smp mode, with 1 process with 2 PEs (worker threads): +p2  ++ppn 2 
  2. smp mode, with 2 processes with 1 worker thread (PE) each, +p2 ++ppn 1
  3. non smp mode (used a different build). +p2

1 and 3 behave similarly, and with sort of expected behavior (with the exception of some gaps.. could be because I was running it on my imac, not a dedicated cluster node). 2, smp with comm threads, performs much poorly, mostly because of 20-60 ms "stretches" in a benchmark that otherwise takes 7 ms if everything goes ok.

Another observation based on smp ++ppn 2 case (best of the 3): sender needs longer time than the receiver per message. by about 50%. Somewhat surprising, since its just mmaking the call to the aggregation library. Maybe some virtual method lookup (or loop counters/tests are expensive... unlikely).

Another tram mystery that needs to be tracked: although maxItems is 1900, which should lead to a message size of 1900*4 + 1 envelope, it actually sends 60kb+ size messages. (does tram send longer messages due to a bug?)

Case 3 should be used to find performance issues (independent of tram) of smp comm thread. For that purpose, I would suggest not using "aggregate" keyword, and sending 10ish 30 kb messages to PE1.


