#include <stdio.h>
#include "hello.decl.h"

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ int totalSends,sendBatch;

/* readonly */ float values[3][3];

template <>
struct is_PUPbytes<int> {
  static const bool value = true;
};

/*mainchare*/
class Main : public CBase_Main
{
 CProxy_Hello arr;
public:
  Main(CkArgMsg* m)
  {
    //Process command-line arguments
    if(m->argc == 3 ) { 
	totalSends=atoi(m->argv[1]);
	sendBatch = atoi(m->argv[2]);
	}
	else
	{
	CkAbort("need 2 arguments after program name\n");
	}	
    delete m;

    //Start the computation
    CkPrintf("Running tramTest on %d processors for 2 elements. totalSize=%d, sendBatch: %d, \n",
	     CkNumPes(),totalSends,sendBatch);
    mainProxy = thisProxy;

   arr = CProxy_Hello::ckNew(2);

  };

  void done(void)
  {
  
    arr[0].startSends();
  };
};

/*array [1D]*/
class Hello : public CBase_Hello 
{
int count;
public:
  Hello()
  {
    CkPrintf("[%d] Hello %d created\n", CkMyPe(), thisIndex);
    count = 0;
    CkCallback cb(CkReductionTarget(Main, done), mainProxy);
    contribute(cb);
}
  Hello(CkMigrateMessage *m) {}
  
  void startSends()
  {
   if (thisIndex == 0) {
     for (int i = 0; i < totalSends; i += sendBatch) {
      for (int j = 0; j < sendBatch; ++j)
	{ 
	thisProxy[1].ping(i+j); 
	}
//      CkPrintf("Batch upto % d sent. Yielding\n", i);
      CthYield(); 
   }
    CkPrintf("[%d] done sending from element %d\n", CkMyPe(),  thisIndex);
  }
}

  void ping(int payload) {
	if (++count == totalSends) thisProxy[0].ack();
   }

  void ack() {
    CkPrintf("done. Exiting.\ni");
    CkExit();
  }
};

#include "hello.def.h"
