#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "zType_Def.h"
#include "zSalOS.h"
#include "zTraceApi.h"

#include "zFastQue.h"
#include "zFastQueApi.h"
#include "zFootprintApi.h"

#include "zGlobal.h"

void* zFastQueInitEx(const char* name, int maxCount, int maxSize, const char* function, int line)
{
  int len = sizeof(FastQue_t) + (maxCount+1)*(maxSize+1);
  FastQue_t *fastQ = malloc(len);
  if(!fastQ)
  {
    zTraceAlarm("memory overflow, bytes: %d.\n", len);
    return 0;
  }
  
  memset(fastQ, 0, len);
  
  FASTQ_SEM_INIT(fastQ);
  fastQ->maxEnding = (maxCount+1)*(maxSize+1);
  fastQ->maxSize = maxSize;

  fastQ->name = name;
  //fastQ->stat.??? = 0;

  zServiceCoreAdd(fastQ, eServiceQUE, name, function, line);

  return fastQ;
}


int zFastEnqueEx(void *que, void *pMsg, int size, const char* function, int line)
{ 
  int ret;
  int next;

  FastQue_t *fastQ = (FastQue_t *)que;
  if(!pMsg || !fastQ || !size)
  {
    zTraceWarn(" @%s:%d invalid null input queue %p or pMsg %p or size %d.\n", function, line, fastQ, pMsg, size);
    return -1;
  }

  ret = -1;
  FASTQ_SEM_WAIT(fastQ);
  {
    next = fastQ->rear + fastQ->maxSize + 1;
    if(next >= fastQ->maxEnding) next = 0;
    
    if(next != fastQ->head)
    {
      fastQ->data[fastQ->rear] = MIN(fastQ->maxSize, size);
      ret = fastQ->data[fastQ->rear];
      
      memcpy(&fastQ->data[fastQ->rear+1], pMsg, fastQ->data[fastQ->rear]);
      fastQ->rear = next;

      fastQ->stat.enq_succ += 1;

      //zTraceDebugQ(" @%s:%d enque %p :", function, line, que); zTraceHexDebugQ(pMsg, size); zTraceDebugQ("\n\n");
      int depth = fastQ->rear - fastQ->head;
      if(depth < 0) depth += fastQ->maxEnding;
      if(fastQ->stat.high_watermark < depth) 
      {
        fastQ->stat.high_watermark = depth;
        //zTraceP("QUEUE %s, rear: %d, head: %d, highwatermark: %d\n", fastQ->name, fastQ->rear, fastQ->head, fastQ->stat.high_watermark);
      }
    }
    else
    {
      zTraceWarn(" @%s:%d FastQ %p overflow.\n", function, line, que);
      fastQ->stat.enq_drop += 1;
      
      ret = -1;
    }
  }
  FASTQ_SEM_POST(fastQ);
  
  return ret;
}



int zFastDequeEx(void *que, void* pMsg, int size, const char* function, int line)
{ 
  FastQue_t *fastQ = (FastQue_t *)que;

  if(!pMsg || !fastQ || !size)
  {
    zEosPeg("bad-input", 0, 0, function, line);
    zTraceWarn(" @%s:%d invalid null input queue %p or pMsg %p, or size %d.\n", function, line, fastQ, pMsg, size);
    return -1;
  }

  //since there must be only 1 consumer, we don't need lock "head"... 
  if(fastQ->head == fastQ->rear)
  {
    zEosPeg("queue-empty", 0, 0, function, line);
    return 0;
  }

  size =  MIN(size, fastQ->data[fastQ->head]);
  memcpy(pMsg, &fastQ->data[fastQ->head+1], size);

  fastQ->head += fastQ->maxSize + 1;
  if(fastQ->head >= fastQ->maxEnding) fastQ->head = 0;

  zEosPeg("pop-succ", 0, 0, function, line);
  //zTraceDebugQ(" @%s:%d deque %p :", function, line, que); zTraceHexDebugQ(pMsg, size); zTraceDebugQ("\n\n");

  fastQ->stat.deq_succ += 1;
  
  return size;
}


int zFastQueShow(void *que)
{
  FastQue_t *fastQ = (FastQue_t *)que;

  if(!fastQ)
  {
    zTraceP("invalid null input queue %p.\n", fastQ);
    return -1;
  }

  zTraceP("  FAST Que %p:\n", fastQ);
  zTraceP("   .name      = %s\n", fastQ->name);  
  zTraceP("   .maxSize   = %d\n", fastQ->maxSize);
  zTraceP("   .rear      = %d\n", fastQ->rear);
  zTraceP("   .head      = %d\n", fastQ->head);
  zTraceP("   .maxEnding = %d\n", fastQ->maxEnding);

  zTraceP("   ..maxCount = %d\n", fastQ->maxEnding / (fastQ->maxSize+1) - 1);  
  zTraceP("   ..usedCount= %d\n", ABS(fastQ->rear - fastQ->head)/(fastQ->maxSize+1));

  
  zTraceP("   .high_watermark= %d (%d)\n", fastQ->stat.high_watermark, fastQ->stat.high_watermark/(fastQ->maxSize+1));

  zTraceP("   .enq_succ      = %d\n", fastQ->stat.enq_succ);
  zTraceP("   .enq_drop      = %d\n", fastQ->stat.enq_drop);
  zTraceP("   .enq_err       = %d\n", fastQ->stat.enq_err);
  zTraceP("   .deq_succ      = %d\n", fastQ->stat.deq_succ);
  zTraceP("   .deq_drop      = %d\n", fastQ->stat.deq_drop);
  zTraceP("   .deq_err       = %d\n", fastQ->stat.deq_err);

  return 0;
}

int zFastQueStatReset(void *que)
{
  FastQue_t *fastQ = (FastQue_t *)que;

  if(!fastQ)
  {
    zTraceP("invalid null input queue %p.\n", fastQ);
    return -1;
  }

  memset(&fastQ->stat, 0, sizeof(fastQ->stat));

  return 0;
}

#define FASTQUE_SELFTEST

#ifdef FASTQUE_SELFTEST

int testFastQue()
{
  void* qid = zFastQueInit(TEST_QUE, 10, sizeof(zMsgHdr_t));
  if(!qid)
  {
    zTraceP("failed to init que.\n");
  }
  zTraceP("qid: %p\n", qid);

  zMsgHdr_t tempMsg;

  tempMsg.msgHandle = 0;

  int ret;
  
  tempMsg.srcAddr = 1;  tempMsg.dstAddr = 2;
  ret = zFastEnque(qid, &tempMsg, sizeof(tempMsg));
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);

  tempMsg.srcAddr = 3;  tempMsg.dstAddr = 4;
  ret = zFastEnque(qid, &tempMsg, sizeof(tempMsg));
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);

  zFastQueShow(qid);

  ret = zFastDeque(qid, &tempMsg, sizeof(tempMsg));
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);
  
  ret = zFastDeque(qid, &tempMsg, sizeof(tempMsg));
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);

  ret = zFastDeque(qid, &tempMsg, sizeof(tempMsg));
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);
  
  return 0;
}

#endif/*FASTQUE_SELFTEST*/

