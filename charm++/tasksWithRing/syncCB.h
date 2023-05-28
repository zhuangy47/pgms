
// *****************************************************************************************
class SyncCB {  // This should be added Charm++ as a utility.. after some renaming/cleanup (AtomicWithCallback?)
 public:
  CkCallback cb;
  std:: atomic<int> remainingTasks;
  SyncCB(CkCallback _cb, int startingTasks) {
    cb = _cb;
    remainingTasks = startingTasks; // lookup atomic vbl initialization
  }

  void increment(int k) {
    // k new tasks have been created. atomically increment remainingTasks by k
    atomic_fetch_add(&remainingTasks, k);  
  }

  void decrement() {
    // atomically (fetch and?) decrement remainingTasks (we assume we finish tasks one by one.. but ok to generalize to k
    int old = atomic_fetch_sub(&remainingTasks, 1);
    //  ckout << "old remainingTasks: " << old << endl;
    if (1 == old) // that means remainingTasks became 0 after the decrement
      cb.send();
  }
  // should we add a subtract method? but will have to check for the "exception" of it going negative.
};

