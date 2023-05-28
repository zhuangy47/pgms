#include <stdio.h>
#include "hello.decl.h"

#include <syncCB.h>

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
    else CkAbort("3 arguments needed: baseLoad, maxLoad, tasksPerChare\n");

    ckout << "baseload: " << baseLoad << "maxLoad: " << maxLoad << ", tasksPerChare:" << tasksPerChare << endl;
    delete m;
    taskEventID = traceRegisterUserEvent("Task execution"); // for projections visualization
    numChares= 2* CkNumPes(); // 2 chares on each PE. 
    mainProxy = thisProxy;
    taskRunner = CProxy_TasksAndRing::ckNew(numChares);
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
//                                          TaskQ method

struct ConverseTaskMsg {
  char converseHdr[CmiMsgHeaderSizeBytes]; // for system use
  int num;
  // atomic int  reference to increment the result
  SyncCB * sync;
};

void taskHandler(void *m1)
{
  ConverseTaskMsg * m = (ConverseTaskMsg *) m1;
  //ckout << "in taskHandler" << m->loadLow << ":" << m->loadHigh << endl;
  //  ckout << m->sync->cb << endl; can we print a callback?
  if (m->num <2 ) // There is only 1 task. Do it.
    {
      traceBeginUserBracketEvent(taskEventID);
      doWork(m->loadLow);
      traceEndUserBracketEvent(taskEventID);
      SyncCB * s =  m->sync;
      // later.. if i == 1, increment the number whose address is provided in the message. 
      s->decrement(); // decrement atomically, and if it becomes 0, send callback to the originator chare.
      CmiFree(m);
    }
  else
    { // fire 2 tasks with the range divided approximately equally
      // loadLow:mid, mid+1:loadHigh. Reuse 1 message.
      int n1 = num-1; int num2 = num -2;
      ConverseTaskMsg * m2 = (ConverseTaskMsg *) CmiAlloc(sizeof(ConverseTaskMsg)) ;
      m2->num = m->num - 2;
      m2->sync = m->sync;
      m2->sync->increment(1);
      CmiSetHandler(m2, taskHandlerIndex);
      CsdTaskEnqueue(m2);

      m->num--;
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
class TasksAndRing : public CBase_TasksAndRing 
{
 public:
  int count;
  int nextIndex;
  SyncCB * s;
 TaskAndRing()
  {
    CkPrintf("[%d] Client %d created\n", CkMyPe(), thisIndex);
    count = 0;
    allTasksDone = 0;
    nextIndex = thisIndex +1;
    if (nextIndex >= numChares) nextIndex = 0;
    thisProxy[nextIndex].forwardRing();
  }

  TaskAndRing(CkMigrateMessage *m) {}

  // create tasks that recursively compute first FIBMAX Fibonacci/Pingala numbers
  // while concurrently serving a ring message going around the chare array
  // the ring message should be higher priority above tasks. So, tasks (beyond the currently running one)
  // does not delay the ring.

  void forwardRing()
  {
    count++;
    if ( (0 == thisindex) &&  (allTasksDone) ) 
      {
	ckout << " numer of rings completed is : " << count << endl;
	CkExit();
      }
    else
      thisProxy[nextIndex].forwardRing();
  }    

 void done() {
   allTasksDone = 1;
  }

 void startTasks()
 { // for now, just do the work inline
   int r = thisIndex; 
   int k = tasksPerChare; // should be a command line para. Each chare creates 4 (or k) tasks
    for (int i=0; i<k; i++) 
      doWork(k*r+i);
    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(0, NULL, CkReduction::nop, cb);    
 }

// *****************************************************************************************
  void fireViaTaskQ()
  {
    CkCallback cb(CkIndex_TaskAndRing::taskQTasksCompleted() , thisProxy[thisIndex]);
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
