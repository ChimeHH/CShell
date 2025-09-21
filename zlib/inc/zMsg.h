/*----------------------------------------------------------
File Name  : xxx.h
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_MSGQUE_H__
#define __Z_MSGQUE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MSG_TYPE_
{
  zAddr_t srcAddr;
  zAddr_t dstAddr; 
  zHandle_t msgHandle;  
} PACKED zMsg_t;

typedef struct Z_UDP_ADDR_TYPE
{
  dword_t ip;
  word_t  port;  

  FuncAddr2NextHop_t fnAddr;
} zNextHopAddr_t;
typedef struct Z_MSGQ_ADDR_TYPE
{
  void *qid;
  
  FuncAddr2Qid_t fnAddr;
} zQueAddr_t;


typedef struct Z_MSGQ_OUT_TYPE
{
  zAddr_t addr;  
  AddrCompare_t fnCompare;
  
  zNextHopAddr_t nextHop;
  zQueAddr_t queAddr;  
} zMsgRoute_t;


#define MAX_MSG_CYCLE   300 //any message mustn't be cached more than 300 seconds
typedef struct MSG_BUFF_HDR_T
{
  const char* function;
  dword_t line:16;
  dword_t used:1; //only for debug
  dword_t rsvd:15;
  
  int tAlloc;
} PACKED BlkHdr_t;

typedef struct MSG_POOL_MGR_T
{  
  int shmKey;
  int dataSize; //actual size limitation
  
  int shmId;
  int shmTotal;
  
  int blkCount;
  int blkWidth; //actual block width
  int blkNext;
  void *blkTable;
} MsgPoolMgr_t;

#define MSG_HANDLE(index, time) (index + ((time & 0x7F)<<24)) //high 8 bits are magic (actual 7bits)
#define MSG_INDEX(handle) (handle & 0xFFFFFF) //low 24 bits
#define MSG_MAGIC_VALID(handle, time) ((handle>>24) == (time & 0x7F))




typedef struct Z_MSG_QUEUE_TABLE_TYPE
{  
  sem_t sem;

  Udp2QueMsgHook_t hookUdp2Que;
  Que2UdpMsgHook_t hookQue2Udp;
  
  MsgPoolMgr_t msgPool;

  int  maxRoute;
  int  busyRoute;
  zMsgRoute_t listRoute[1];
} zMsgMgr_t;

#define MSG_SEM_INIT(pMgr) sem_init(&(((zMsgMgr_t *)pMgr)->sem), 0, 1);
#define MSG_SEM_WAIT(pMgr) sem_wait(&(((zMsgMgr_t *)pMgr)->sem))
#define MSG_SEM_POST(pMgr) sem_post(&(((zMsgMgr_t *)pMgr)->sem))
#define MSG_SEM_DEL(pMgr)  do{}while(0)


#ifdef __cplusplus
}
#endif

#endif /*__Z_MSGQUE_H__*/






