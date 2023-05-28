#include <stdio.h>
#include "hello.decl.h"

/* readonly */ int tasksPerChare = 4;
/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ int numChares;
/* readonly*/ CProxy_NodeTasks nodeMgr;
/* readonly */ double baseLoad;  // both input in integer microseconds
/* readonly */ double maxLoad;  
/* readonly */ int taskEventID = 0;

int taskHandlerIndex = 0;
// one of exceptions.. this is not readonly, but meant for the converse functions, not Charm++, so it is ok. 

/*mainchare*/
class Main : public CBase_Main
{
  Main_SDAG_CODE
  CProxy_Hello taskRunner;
  double t0, t1, t2, t3; 
 public:
  Main(CkArgMsg* m)
  {
    if(m->argc == 4 ) {     //Process command-line arguments
	baseLoad=0.000001* atoi(m->argv[1]);
	maxLoad = 0.000001 *atoi(m->argv[2]);
	tasksPerChare = atoi(m->argv[3]);
    }
    else CkAbort("3arguments needed: baseLoad, maxLoad, tasksPerChare\n");

    ckout << "baseload: " << baseLoad << "maxLoad: " << maxLoad << ", tasksPerChare:" << tasksPerChare << endl;
    delete m;
    taskEventID = traceRegisterUserEvent("Task execution"); // for projections visualization
    numChares= 2* CkNumPes(); // 2 chares on each PE. 
    mainProxy = thisProxy;
    nodeMgr = CProxy_NodeTasks::ckNew();
    taskRunner = CProxy_Hello::ckNew(numChares);
    thisProxy.run();  // defined as an sdag methid in the .ci file
  };
};

// the dummy TASK...
float doWork(int j) {
 // do dummy work proportional to j microseconds, between baseLoad and maxLoad;
  float y = 1.0;
  float duration = baseLoad + (maxLoad - baseLoad)*j/(tasksPerChare*numChares);
  double t0 = CkTimer();
  while ((CkTimer() - t0) < duration)  y += 1.0 + 1/y; // just a dyummy calculation
  // calculate and return y, so the compiler doesn't optimize it away.
  return y;
}

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

// *****************************************************************************************
//                                          NodeGroup Method
class TaskMsg : public CMessage_TaskMsg {
 public:
  int load;
  SyncCB * sync;
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

// *****************************************************************************************
//                                          TaskQ method

struct ConverseTaskMsg {
  char converseHdr[CmiMsgHeaderSizeBytes]; // for system use
  int numTasks;
  int loadLow;
  int loadHigh;
  SyncCB * sync;
};

void taskHandler(void *m1)
{
  ConverseTaskMsg * m = (ConverseTaskMsg *) m1;
  //ckout << "in taskHandler" << m->loadLow << ":" << m->loadHigh << endl;
  //  ckout << m->sync->cb << endl; can we print a callback?
  if (m->loadLow == m->loadHigh) //// There is only 1 task. Do it.
    {
      traceBeginUserBracketEvent(taskEventID);
      doWork(m->loadLow);
      traceEndUserBracketEvent(taskEventID);
      SyncCB * s =  m->sync;
      s->decrement(); // decrement atomically, and if it becomes 0, send callback to the originator chare.
      CmiFree(m);
    }
  else
    { // fire 2 tasks with the range divided approximately equally
      // loadLow:mid, mid+1:loadHigh. Reuse 1 message.
      int mid = (m->loadLow+m->loadHigh)/2.0;
      if (mid+1 <= m->loadHigh) // sanity check..
	{
	  ConverseTaskMsg * m2 = (ConverseTaskMsg *) CmiAlloc(sizeof(ConverseTaskMsg)) ;
	  m2->loadLow = mid+1;
	  m2->loadHigh = m->loadHigh;
	  m2->numTasks = m2->loadHigh - m->loadLow +1; // recall.. range is inclusive
	  
	  m2->sync = m->sync;
	  m2->sync->increment(1);
	  CmiSetHandler(m2, taskHandlerIndex);
	  CsdTaskEnqueue(m2);
	}
      m->loadHigh = mid;
      m->numTasks = m->loadHigh - m->loadLow +1; // recall.. range is inclusive
      //   m->sync->decrement(); m->sync->increment(1); cancel out. 
      CsdTaskEnqueue(m);
    }
}

void taskHandlerInitcall()
// Register taskHandler as a  converse Handler
{
  ckout << "in initcall" <<endl;
  taskHandlerIndex = CmiRegisterHandler(taskHandler);
}

// *****************************************************************************************
//                                   Client chare array

/*array [1D]*/
class Hello : public CBase_Hello 
{
 public:
  SyncCB * s;
 Hello()
  {
    CkPrintf("[%d] Client %d created\n", CkMyPe(), thisIndex);
  }

  Hello(CkMigrateMessage *m) {}
  
   // chare with rank r create k tasks, each doing work proportional to baseLoad + (k*r..(k+1)r-1) *maxLoad-baseLoad)   
    // 3 ways of creating tasks:
    // 1. do it yourself  (doTasks)
    // 2. enqueue them in the node queue (fireNodeTasks), completion callback to nodeTasksCompletged (via SyncCB)
    // 3. enqueue them in the taskq (fireViaTaskQ),  completion callback to taskQTasksCompletged (via SyncCB)

  void doTasks()
  {
   int r = thisIndex; 
   int k = tasksPerChare; // should be a command line para. Each chare creates 4 (or k) tasks
    for (int i=0; i<k; i++) 
      doWork(k*r+i);
    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(0, NULL, CkReduction::nop, cb);    
 }
// *****************************************************************************************
  void fireNodeTasks()
    {
      int r = thisIndex;
      CkCallback cb(CkIndex_Hello::nodeTasksCompleted() , thisProxy[thisIndex]);
      s = new SyncCB( cb, tasksPerChare); // note is a variable at the chare level
      for (int i=0; i<tasksPerChare; i++)
	{
	  TaskMsg * m = new TaskMsg(); m->load = tasksPerChare*r+i; m->sync = s;
	  nodeMgr[CkMyNode()].doNodeTask(m);
	}
    }

  // method to catch when the count becomes 0
  void nodeTasksCompleted() {
    delete s;
    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(0, NULL, CkReduction::nop, cb);    
  }
// *****************************************************************************************
  void fireViaTaskQ()
  {
    CkCallback cb(CkIndex_Hello::taskQTasksCompleted() , thisProxy[thisIndex]);
    s = new SyncCB( cb, 1); // just firing 1 task, but it will recursively fire more. 
    ConverseTaskMsg * msg = (ConverseTaskMsg *) CmiAlloc(sizeof(ConverseTaskMsg)) ;
    msg->loadLow = tasksPerChare*thisIndex;
    msg->loadHigh = msg->loadLow+ tasksPerChare -1; // inclusive range
    msg->sync = s; //  the converse handler wil do cb.send.
    msg->numTasks = tasksPerChare; // numTasks is not needed in this formulation
    CmiSetHandler(msg, taskHandlerIndex);
    // firing 1 task, which will be recursivelu subdivided to tasksPerchare real tasks
    CsdTaskEnqueue(msg);
  }

  void taskQTasksCompleted()
  {
    delete s;
    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(0, NULL, CkReduction::nop, cb);    
  }
};

#include "hello.def.h"
