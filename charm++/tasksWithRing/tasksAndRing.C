#include <stdio.h>
#include "tasksAndRing.decl.h"

#include "syncCB.h"

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ int numChares;
/* readonly */ int fibmax = 15; 
int taskHandlerIndex = 0;
// one of exceptions.. this is not readonly, but meant for the converse functions, not Charm++, so it is ok. 

/*mainchare*/
class Main : public CBase_Main
{
  Main_SDAG_CODE
  CProxy_TasksAndRing taskRunner;
  double t0, t1, t2, t3; 
 public:
  Main(CkArgMsg* m)
  {
    ckout <<"starting" <<endl;
    if(m->argc == 2 ) {     //Process command-line arguments
	fibmax = atoi(m->argv[1]);
    }
    else CkAbort("1 argument needed: fibmax: each chare fires fibmax recursive tasks\n");

    ckout << "fibmax " << fibmax  << endl;
    delete m;
    numChares= 2* CkNumPes(); // 2 chares on each PE. 
    mainProxy = thisProxy;
    taskRunner = CProxy_TasksAndRing::ckNew(numChares);
    taskRunner.startTasks();
  };
 };

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
  ckout << "in taskHandler"  << m->num << endl;
  if (m->num <2 ) // There is only 1 task. Do it.
    {
      SyncCB * s =  m->sync;
      // later.. if i == 1, increment the number whose address is provided in the message. 
      s->decrement(); // decrement atomically, and if it becomes 0, send callback to the originator chare.
      CmiFree(m);
    }
  else
    { // fire 2 tasks for m->num -1 and m->num - 2 
      ckout << "i the else clause to fire recursive subtasks" << endl;
      ConverseTaskMsg * m2 = (ConverseTaskMsg *) CmiAlloc(sizeof(ConverseTaskMsg)) ;
      m2->num = m->num - 2;
      m2->sync = m->sync;
      m2->sync->increment(1);
      CmiSetHandler(m2, taskHandlerIndex);
      CsdTaskEnqueue(m2); // ***** fire task for num-2

      m->num--;
      //   m->sync->decrement(); m->sync->increment(1); cancel out. 
      CsdTaskEnqueue(m); // ****** fire task for num-1
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
  int allTasksDone = 0;
  TasksAndRing()
  {
    CkPrintf("[%d] Client %d created\n", CkMyPe(), thisIndex);
    count = 0;
    allTasksDone = 0;
    nextIndex = thisIndex +1;
    if (nextIndex >= numChares) nextIndex = 0;
    if (0 == thisIndex) thisProxy[nextIndex].forwardRing();
  }

  TasksAndRing(CkMigrateMessage *m) {}
  // create tasks that recursively compute first FIBMAX Fibonacci/Pingala numbers
  // while concurrently serving a ring message going around the chare array
  // the ring message should be higher priority above tasks. So, tasks (beyond the currently running one)
  // does not delay the ring.

  void forwardRing()
  {
    count++;
    ckout << thisIndex <<  ": in forwardring " << ":count=" << count << endl;
    if ( (0 == thisIndex) &&  (allTasksDone==1) ) 
      {	ckout << thisIndex << ": numer of rings completed is : " << count << "allTasksDone:"<< allTasksDone << endl;
	CkExit();
      }
    else
      { ckout << thisIndex << ": forwarding to " << nextIndex << endl;
      thisProxy[nextIndex].forwardRing();
      }
  }    

 void done() {
   ckout << thisIndex << ":all tasks done." << endl;
   allTasksDone = 1;
  }

 void startTasks()
 { // for now, just do the work inline
   int r = thisIndex;
   CkCallback cb(CkIndex_TasksAndRing::tasksCompleted() , thisProxy[thisIndex]);
   s = new SyncCB(cb,fibmax);
   for (int i=0; i<fibmax; i++) 
     {
      ConverseTaskMsg * m2 = (ConverseTaskMsg *) CmiAlloc(sizeof(ConverseTaskMsg)) ;
      m2->num = i;
      m2->sync = s ;
      CmiSetHandler(m2, taskHandlerIndex);
      CsdTaskEnqueue(m2); // ***** fire task
     }   
   ckout << thisIndex << ": in startTasks. started them" << endl;

 }

 /*
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
 */
  void tasksCompleted()
  {
    ckout << thisIndex << ": in tasksCompled " << endl;
    delete s;
    CkCallback cb(CkReductionTarget(TasksAndRing, done), thisProxy[0]);
    contribute(0, NULL, CkReduction::nop, cb);    
  }
};

#include "tasksAndRing.def.h"
