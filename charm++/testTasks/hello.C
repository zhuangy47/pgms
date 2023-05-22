#include <stdio.h>
#include "hello.decl.h"

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ int nElements;

/* readonly */ float values[3][3];

/*mainchare*/
class Main : public CBase_Main
{
public:
  Main(CkArgMsg* m)
  {
    //Process command-line arguments
    nElements=5;
    if(m->argc >1 ) nElements=atoi(m->argv[1]);
    delete m;

    //Start the computation
    CkPrintf("Running Hello on %d processors for %d elements\n",
	     CkNumPes(),nElements);
    mainProxy = thisProxy;

    CProxy_Hello arr = CProxy_Hello::ckNew(nElements);

    arr[0].SayHi(17);
  };

  void done(void)
  {
    CkPrintf("All done\n");
    CkExit();
  };
};

float doWork(int j) {
 // do dummy work proportional to j*T0 microseconds;
  float y = 0;
  double t0 = CkTimer();
  while ((CkTimer() - t0) < 0.000001*j)  y += 1.0;
  return y;
}

/*array [1D]*/
class Hello : public CBase_Hello 
{
public:
 Hello()
  {
    CkPrintf("[%d] Hello %d created\n", CkMyPe(), thisIndex);
  }

  Hello(CkMigrateMessage *m) {}
  
  void doTasks()
  {
   int r = thisIndex; 
   // chare with rank r, create k tasks, each doing work proportional to (base+k*r)..(base+(k+1)r   

    CkPrintf("[%d] Hi[%d] from element %d\n", CkMyPe(), hiNo, thisIndex);
    // 3 ways of creating tasks: do it yourself, enqueue them in the node queue, enqueue them in the taskq.
    for (int i=0; i<k; i++) 
       doWork(base+k*r+i);
    // contribute to a callback to doneExperiment (we could use a threaded method in main, and resume on reduction
 }

/* 
  void nodeqTasks()
    {
     int r = thisIndex;
    for (int i=0; i<k; i++)
      { 
       // create a message, fill in task description.. which is just the work number: base+k*r+i; and ref to the atpmic count
       //  enqueue in the node queue 
       // set an atomic vbl that will be incremented on task creation and decrement on task completion.
       // declare the atomic as a member  of this chare, and send a reference to it in the task descriptor
      }
    }
*/
  // method to catch when the count becomes 0

// similar method for the taskq. 

};

#include "hello.def.h"
