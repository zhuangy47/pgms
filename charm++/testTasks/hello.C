#include <stdio.h>
#include "hello.decl.h"

#define TASKS_PER_CHARE 4
/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ int numChares;
/* readonly*/ CProxy_NodeTasks nodeMgr;
/* readonly */ double baseLoad;  // both input in integer microseconds
/* readonly */ double maxLoad;  


/*mainchare*/
class Main : public CBase_Main
{
  Main_SDAG_CODE
  CProxy_Hello taskRunner;
  double t0, t1; 
 public:
  Main(CkArgMsg* m)
  {
    //Process command-line arguments

    if(m->argc == 3 ) { 
	baseLoad=0.000001* atoi(m->argv[1]);
	maxLoad = 0.000001 *atoi(m->argv[2]);
    }
    else CkAbort("2 arguments needed");

    ckout << "baseload: " << baseLoad << "maxLoad: " << maxLoad << endl;
    delete m;
    numChares= 2* CkNumPes(); // 2 chares on each PE. 
    
    mainProxy = thisProxy;
    nodeMgr = CProxy_NodeTasks::ckNew();

   taskRunner = CProxy_Hello::ckNew(numChares);

   thisProxy.run();
  };

};

float doWork(int j) {
 // do dummy work proportional to j*T0 microseconds;
  float y = 1.0;
  float duration = baseLoad + (maxLoad - baseLoad)*j/(TASKS_PER_CHARE*numChares);
  //  ckout << "task with duration = " << duration << "seconds" << endl;
  double t0 = CkTimer();
  while ((CkTimer() - t0) < duration)  y += 1.0 + 1/y; // just a dyummy calculation
  // so the compiler doesn't optimize it away.
  return y;
}

class SyncCB {
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
    // ckout << "old remainingTasks: " << old << endl;
    if (1 == old) // that means remainingTasks became 0 after the decrement
      cb.send();
  }
};

class TaskMsg : public CMessage_TaskMsg {
 public:
  int load;
  SyncCB * sync; // may be void *?
};

// Nodegroup
class NodeTasks : public CBase_NodeTasks
{
 public:
  NodeTasks()
    {ckout << "nodegroup constructed on node: " << CkMyNode() << endl;
    }

  void doNodeTask(TaskMsg * m) {
    // later: divide the task and enqueue subtasks rcursively to do it like steal queues for low overhead.
    SyncCB * s =  m->sync;
    doWork(m->load);
    s->decrement(); // decrement atomically, and if it becomes 0, send callback to the originator chare.
    CkFreeMsg(m);
  }
};

/*array [1D]*/
class Hello : public CBase_Hello 
{
 
 public:
  SyncCB * s;
 Hello()
  {
    CkPrintf("[%d] Hello %d created\n", CkMyPe(), thisIndex);
  }

  Hello(CkMigrateMessage *m) {}
  
  void doTasks()
  {
   int r = thisIndex; 
   // chare with rank r, create k tasks, each doing work proportional to (base+k*r)..(base+(k+1)r   
   int k = TASKS_PER_CHARE; // should be a command line para. Each chare creates 4 (or k) tasks
    // 3 ways of creating tasks: do it yourself, enqueue them in the node queue, enqueue them in the taskq.
    for (int i=0; i<k; i++) 
      doWork(k*r+i);

    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(0, NULL, CkReduction::nop, cb);    
    // contribute to a callback to doneExperiment (we could use a threaded method in main, and resume on reduction
 }
 
  void fireNodeTasks()
    {
      int r = thisIndex;
      CkCallback cb(CkIndex_Hello::nodeTasksCompleted() , thisProxy[thisIndex]);
      s = new SyncCB( cb, TASKS_PER_CHARE);
    for (int i=0; i<TASKS_PER_CHARE; i++)
      {
	TaskMsg * m = new TaskMsg(); m->load = TASKS_PER_CHARE*r+i; m->sync = s;
	nodeMgr[CkMyNode()].doNodeTask(m);
	// later... change this so it enqueues a range of chares, which is recursively subdivided.
       // create a message, fill in task description..; and ref to the atomic count
       //  enqueue in the node queue 
       // set an atomic vbl that will be incremented on task creation and decrement on task completion.
       // declare the atomic as a member  of this chare, and send a reference to it in the task descriptor
      }
    }

  // method to catch when the count becomes 0
  void nodeTasksCompleted() {
    //    ckout << "nodeTasksCompleted for Worker [ " << thisIndex << "]" << endl;
    delete s;
    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(0, NULL, CkReduction::nop, cb);    
  }

// similar method for the taskq. 

};

#include "hello.def.h"
