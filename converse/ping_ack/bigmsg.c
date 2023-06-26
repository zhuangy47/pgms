#include <stdio.h>
#include <time.h>
#include <converse.h>
#include "bigmsg.cpm.h"

CpvDeclare(int, bigmsg_index);
CpvDeclare(int, recv_count);
CpvDeclare(int, ack_count);

#define MSG_SIZE 8
#define MSG_COUNT 100

typedef struct myMsg
{
  char header[CmiMsgHeaderSizeBytes];
  int payload[MSG_SIZE];
} *message;

CpmInvokable bigmsg_stop()
{
  CsdExitScheduler();
}


void bigmsg_handler(void *vmsg)
{
  int i, next;
  message msg = (message)vmsg;
  if (CmiMyPe()>=CmiNumPes()/2) {
    CpvAccess(recv_count) = 1 + CpvAccess(recv_count);
    if(CpvAccess(recv_count) == MSG_COUNT) {
      CmiPrintf("\nTesting recvd data on rank %d", CmiMyRank());
      for (i=0; i<MSG_SIZE; i++) {
        if (msg->payload[i] != i) {
          CmiPrintf("Failure in bigmsg test, data corrupted.\n");
          exit(1);
        }
      }
      CmiFree(msg);
      msg = (message)CmiAlloc(sizeof(struct myMsg));
      for (i=0; i<MSG_SIZE; i++) msg->payload[i] = i;
      CmiSetHandler(msg, CpvAccess(bigmsg_index));
      CmiSyncSendAndFree(0, sizeof(struct myMsg), msg);
    } else
      CmiFree(msg);
  } else { //Pe-0 receives all acks
    CpvAccess(ack_count) = 1 + CpvAccess(ack_count);
    if(CpvAccess(ack_count) == CmiNumPes()/2) {
      CmiPrintf("\nReceived ack on PE-#%d", CmiMyPe());
      CmiFree(msg);
      Cpm_bigmsg_stop(CpmSend(CpmALL));
    }
  }
}

void bigmsg_init()
{
  int i, k;
  struct myMsg *msg;
  int totalpes = CmiNumPes(); //p=num_pes
  int pes_per_node = totalpes/2; //q=p/2
  if (CmiNumNodes()<2) {
    CmiPrintf("note: this test requires at least 2 nodes, skipping test.\n");
    CmiPrintf("exiting.\n");
    CsdExitScheduler();
    Cpm_bigmsg_stop(CpmSend(CpmALL));
  } else {
    if(CmiMyPe() < pes_per_node) {
      CmiPrintf("\nSending msg fron pe%d to pe%d\n",CmiMyPe(), CmiMyNodeSize()+CmiMyRank());
      for(k=0;k<MSG_COUNT;k++) {
//        CmiPrintf("\nSending msg number #%d\n", k);
        msg = (message)CmiAlloc(sizeof(struct myMsg));
        for (i=0; i<MSG_SIZE; i++) msg->payload[i] = i;
        CmiSetHandler(msg, CpvAccess(bigmsg_index));
        //Send from my pe-i on node-0 to q+i on node-1
        CmiSyncSendAndFree(pes_per_node+CmiMyPe(), sizeof(struct myMsg), msg);
      }
    }
  }
}

void bigmsg_moduleinit(int argc, char **argv)
{
  CpvInitialize(int, bigmsg_index);
  CpvInitialize(int, recv_count);
  CpvInitialize(int, ack_count);
  CpvAccess(bigmsg_index) = CmiRegisterHandler(bigmsg_handler);
  void CpmModuleInit(void);
  void CfutureModuleInit(void);
  void CpthreadModuleInit(void);

  CpmModuleInit();
  CfutureModuleInit();
  CpthreadModuleInit();
  CpmInitializeThisModule();
  // Set runtime cpuaffinity
  CmiInitCPUAffinity(argv);
  // Initialize CPU topology
  CmiInitCPUTopology(argv);
  // Wait for all PEs of the node to complete topology init
  CmiNodeAllBarrier();

  // Update the argc after runtime parameters are extracted out
  argc = CmiGetArgc(argv);
  if(CmiMyPe() < CmiNumPes()/2)
    bigmsg_init();
}

int main(int argc, char **argv)
{
  ConverseInit(argc,argv,bigmsg_moduleinit,0,0);
}

