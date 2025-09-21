#include <errno.h>
#include <arpa/inet.h>
#include "zType_Def.h"
#include "zSalOS.h"
#include "zTraceApi.h"
#include "zFastQueApi.h"

#include "zMsgApi.h"
#include "zMsg.h"
#include "cshell.h"

#include "zGlobal.h"

static void* lastServiceCore = 0;
static void zMsgHeadHdrNtoH(zMsgHdr_t *pMsg)
{
  pMsg->srcAddr = ntohl(pMsg->srcAddr);
  pMsg->dstAddr = ntohl(pMsg->dstAddr);
  pMsg->msgId = ntohs(pMsg->msgId);
  pMsg->srcInst = ntohl(pMsg->srcInst);
  pMsg->dstInst = ntohl(pMsg->dstInst);
  return;
}
static void zMsgHeadHdrHtoN(zMsgHdr_t *pMsg)
{
  pMsg->srcAddr = htonl(pMsg->srcAddr);
  pMsg->dstAddr = htonl(pMsg->dstAddr);
  pMsg->msgHandle = htonl(pMsg->msgHandle);
  pMsg->msgLen = htons(pMsg->msgLen);
  pMsg->msgId = htons(pMsg->msgId);
  pMsg->srcInst = htonl(pMsg->srcInst);
  pMsg->dstInst = htonl(pMsg->dstInst);
  return;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
//message share memory pool
int zMsgBuffPoolInit(void *serviceCore, int shmKey, int dataSize, int buffCount, int bResetBuff)
{
  lastServiceCore = serviceCore;
  
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  MsgPoolMgr_t *pPool = &pMgr->msgPool;
  memset(pPool, 0, sizeof(MsgPoolMgr_t));
    
  pPool->shmKey    = shmKey;
  
  pPool->dataSize = dataSize;  //actual size limitation
  pPool->blkWidth  = ROUNDSIZE(dataSize + sizeof(BlkHdr_t));
  
  pPool->blkCount = buffCount + 1; //reserve 0, never use it.
  pPool->blkNext = 1;
  
  pPool->shmTotal = pPool->blkWidth * pPool->blkCount;

  pPool->blkTable = malloc(pPool->shmTotal);
  if(!pPool->blkTable)
  {
    zTraceAlarm("Failed to malloc size %d.\n", pPool->shmTotal);
    return 0;
  }

  return 0;
}

int zMsgSizeMax(void* serviceCore)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  return pMgr->msgPool.dataSize;
}

zHandle_t zMsgNewBuff(void* serviceCore, zMsgHdr_t** ppMsg, const char * function,int line)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  MsgPoolMgr_t *pPool = &pMgr->msgPool;

  
  if(pPool->blkNext <= 0 || pPool->blkNext >= pPool->blkCount) pPool->blkNext = 1;
  int next = pPool->blkNext;

  int tNow = zTime();
  
  int i; 

  MSG_SEM_WAIT(pMgr);
  
  for(i=next; i<pPool->blkCount; i++)
  {
    BlkHdr_t *p = (BlkHdr_t *)(pPool->blkTable + pPool->blkWidth * i);
    if(!p->used || (tNow - p->tAlloc > MAX_MSG_CYCLE))
    {
      if(p->used) EOS_SET("msg-buff-leak-found");

      pPool->blkNext = i+1; //move next

      p->tAlloc = tNow;
      p->function = function;
      p->line = line;
      p->used = 1;
      
      if(ppMsg) *ppMsg = (zMsgHdr_t *)(p+1);

      MSG_SEM_POST(pMgr);

      zTraceMinor("@@@@%s:%d succ serviceCore %p, msgHandle %08x.\n", function, line, serviceCore, MSG_HANDLE(i, tNow));
      return MSG_HANDLE(i, tNow);
    }
  }
  for(i=1; i<next; i++)
  {
    BlkHdr_t *p = (BlkHdr_t *)(pPool->blkTable + pPool->blkWidth * i);
    if(!p->used || (tNow - p->tAlloc > MAX_MSG_CYCLE))
    {
      if(p->used) EOS_SET("msg-buff-leak-found");
      
      pPool->blkNext = i+1; //move next

      p->tAlloc = tNow;
      p->function = function;
      p->line = line;
      p->used = 1;
      
      if(ppMsg) *ppMsg = (zMsgHdr_t *)(p+1);
      
      MSG_SEM_POST(pMgr);

      zTraceMinor("@@@@%s:%d succ serviceCore %p, msgHandle %08x.\n", function, line, serviceCore, MSG_HANDLE(i, tNow));
      return MSG_HANDLE(i, tNow);
    }
  }

  MSG_SEM_POST(pMgr);
  return 0;
}


int zMsgRefreshBuff(void* serviceCore, zHandle_t msgHandle)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr || msgHandle<=0)
  {
    zTraceError("invalid NULL serviceCore %p or invalid msgHandle %08x.\n", serviceCore, msgHandle);
    return -1;
  }
  
  MsgPoolMgr_t *pPool = &pMgr->msgPool;

  int index = MSG_INDEX(msgHandle);

  if(index <= 0 || index >= pPool->blkCount)
  {
    zTraceError("invalid msgHandle %08x, with index: %d.\n", msgHandle, index);
    return -1;
  }
  
  BlkHdr_t *p = (BlkHdr_t *)(pPool->blkTable + pPool->blkWidth * index);
  if(p->used)
  {
    p->tAlloc = zTime();
    return 0;
  }
  
  return -1;
}

int zMsgFreeBuff(void* serviceCore, zHandle_t msgHandle, const char * function,int line)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr || msgHandle<=0)
  {
    zTraceError("@%s:%d invalid NULL serviceCore %p or invalid msgHandle %08x.\n", function, line, serviceCore, msgHandle);
    return -1;
  }

  zTraceMinor("@@@@%s:%d invoke serviceCore %p, msgHandle %08x.\n", function, line, serviceCore, msgHandle);
  
  MsgPoolMgr_t *pPool = &pMgr->msgPool;

  int index = MSG_INDEX(msgHandle);

  if(index <= 0 || index >= pPool->blkCount)
  {
    zTraceError("@%s:%d invalid msgHandle %08x, with index: %d.\n", function, line, msgHandle, index);
    return -1;
  }
  
  BlkHdr_t *p = (BlkHdr_t *)(pPool->blkTable + pPool->blkWidth * index);
    
  MSG_SEM_WAIT(pMgr);
  if(MSG_MAGIC_VALID(msgHandle, p->tAlloc))
  {
    p->tAlloc = 0;
    p->used = 0;
  }
  MSG_SEM_POST(pMgr);

  zTraceMinor("@@@@%s:%d succ serviceCore %p, msgHandle %08x.\n", function, line, serviceCore, msgHandle);

  return 0;
}

zMsgHdr_t* zMsgMapBuffEx(void* serviceCore, zHandle_t msgHandle, int *pLen, int bMagic)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  
  if(!pMgr || msgHandle<=0)
  {
    zTraceError("invalid NULL serviceCore %p or invalid msgHandle %08x.\n", serviceCore, msgHandle);
    return 0;
  }
  
  MsgPoolMgr_t *pPool = &pMgr->msgPool;

  int index = MSG_INDEX(msgHandle);

  if(index <= 0 || index >= pPool->blkCount)
  {
    zTraceError("invalid msgHandle %08x, with index: %d.\n", msgHandle, index);
    return 0;
  }
  
  BlkHdr_t *p = (BlkHdr_t *)(pPool->blkTable + pPool->blkWidth * index);
  if(p->used)
  {
    if(!bMagic || MSG_MAGIC_VALID(msgHandle, p->tAlloc))
    {
      if(pLen) *pLen = pPool->dataSize;
      return (zMsgHdr_t *)(p+1);
    }
  }
  
  return 0;
}

zMsgHdr_t* zMsgMapBuff(void* serviceCore, zHandle_t msgHandle, int *pLen)
{
  return zMsgMapBuffEx(serviceCore, msgHandle, pLen, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//pMsg service init
void* zMsgServiceInit(int maxRoute, int semKey, int shmKey, int dataSize, int buffCount, int bResetBuff)
{
  if(maxRoute<1)
  {
    zTraceFatal("invalid maxRoute %d.\n", maxRoute);
    return 0;
  }

  int size = maxRoute * sizeof(zMsgRoute_t) + sizeof(zMsgMgr_t);
  zMsgMgr_t* pMgr = malloc(size);
  if(!pMgr)
  {
    zTraceFatal("memory downflow, maxRoute: %d.\n", maxRoute);
    return 0;
  }

  memset(pMgr, 0, size);

  
  MSG_SEM_INIT(pMgr);

  int ret = zMsgBuffPoolInit(pMgr, shmKey, dataSize, buffCount, bResetBuff);
  if(ret < 0)
  {
    zTraceFatal("Failed to init pMsg buffer pool, return %d.\n", ret);
    return 0;
  }

  pMgr->maxRoute = maxRoute;

  zServiceCoreAdd(pMgr, eServiceMSG, "MSG", __FUNCTION__, __LINE__);
  
  lastServiceCore = pMgr;

  return pMgr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
//pMsg service route
//throughput-in on addr shall relay to a local qid (i'm receiver)
int zMsgAddrCompareDef(zAddr_t addr0, zAddr_t addr1)
{
  return addr0 - addr1;
}

int zMsgAddRouteEx(void* serviceCore, zAddr_t addr, void *qid, dword_t nextHopIP, word_t nextHopPort, 
                   AddrCompare_t fnCompare, FuncAddr2Qid_t fnQid, FuncAddr2NextHop_t fnNextHop)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  int i;

  if(!addr || !qid)
  {
    zTraceError("Invalid input addr: addr %08x or qid %p.\n", addr, qid);
    return -1;
  }

  if(!fnCompare) fnCompare = zMsgAddrCompareDef;

  MSG_SEM_WAIT(pMgr);
  for(i=0; i<pMgr->busyRoute; i++)
  {
    zMsgRoute_t *route = &pMgr->listRoute[i];
    
    if(fnCompare(route->addr, addr)) continue;
    
    route->fnCompare= fnCompare;
    
    route->queAddr.qid = qid;
    route->queAddr.fnAddr = fnQid;

    route->nextHop.ip = nextHopIP;
    route->nextHop.port = nextHopPort;
    route->nextHop.fnAddr = fnNextHop;
  
    MSG_SEM_POST(pMgr);
    return 1;
  }

  if(pMgr->busyRoute < pMgr->maxRoute)
  {
    zMsgRoute_t *route;
    
    for(i=0; i<pMgr->busyRoute; i++)
    {
      route = &pMgr->listRoute[i];
      if(!route->addr) break;
    }

    if(i == pMgr->busyRoute) pMgr->busyRoute += 1;

    route = &pMgr->listRoute[i];

    memset(route, 0, sizeof(*route));
    
    route->addr = addr; 
    
    route->fnCompare= fnCompare;
    
    route->queAddr.qid = qid;
    route->queAddr.fnAddr = fnQid;

    route->nextHop.ip = nextHopIP;
    route->nextHop.port = nextHopPort;
    route->nextHop.fnAddr = fnNextHop;

    MSG_SEM_POST(pMgr);
    return 1;
  }

  MSG_SEM_POST(pMgr);

  zTraceFatal("zMsg downflow.\n");
  return -1;
}

int zMsgAddRoute(void* serviceCore, zAddr_t addr, void *qid, dword_t nextHopIP, word_t nextHopPort)
{
  return zMsgAddRouteEx(serviceCore, addr, qid, nextHopIP, nextHopPort, 0, 0, 0);
}

int zMsgDelRoute(void* serviceCore, zAddr_t addr)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }


  if(!addr)
  {
    zTraceError("Invalid input addr: %08x.\n", addr);
    return -1;
  }
  
  int i;

  for(i=0; i<pMgr->busyRoute; i++)
  {
    zMsgRoute_t *route = &pMgr->listRoute[i];
    
    if(!route->addr) continue;
    
    if(route->fnCompare(route->addr, addr)) continue;
    
    route->addr = 0;
   
    return 1;    
  }
  return 0;
}

int zMsgSetHook(void* serviceCore, Udp2QueMsgHook_t hookUdp2Que, Que2UdpMsgHook_t hookQue2Udp)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  pMgr->hookUdp2Que = hookUdp2Que;
  pMgr->hookQue2Udp = hookQue2Udp;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//pMsg queue
void* zMsgQueCreate(const char* name, int maxMsgs, const char* function, int line)
{
  void* qid = zFastQueInitEx(name, maxMsgs, sizeof(zMsgHdr_t), function, line);
  if(!qid)
  {
    zTraceFatal("@%s:%d faile to create msgQ %s, maxMsgs = %d.\n", function, line, name, maxMsgs);
    return 0;
  }

  return qid;
}


int zMsgQueRecvEx(void* qid, zMsgHdr_t* pMsg, const char* function, int line) //RETURN: 0 no messages, >0 yes, otherwise error code.
{
  if(!qid || !pMsg)
  {
    zTraceAlarm(" @%s:%d invalid input null que %p or pMsg %p.\n", function, line, qid, pMsg);
    return -1;
  }

  return zFastDequeEx(qid, pMsg, sizeof(zMsgHdr_t), function, line);
}

int zMsgQueSendEx(void* qid, zMsgHdr_t* pMsg, const char* function, int line) //RETURN: <0 on fail.
{
  if(!qid || !pMsg)
  {
    zTraceAlarm(" @%s:%d invalid input null que %p or pMsg %p.\n", function, line, qid, pMsg);
    return -1;
  }

  if(pMsg->msgHandle > 0)
    return zFastEnqueEx(qid, pMsg, sizeof(zMsg_t), function, line);
  else //non-standard local long msgs, e.g. timeout
    return zFastEnqueEx(qid, pMsg, sizeof(zMsgHdr_t), function, line);
}

int zMsgSendEx(void* serviceCore, zMsgHdr_t* pMsg, const char* function, int line)
{
  int i;
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore, @%s,%d.\n", function, line);
    return -1;
  }

  if(!pMsg || !pMsg->dstAddr)
  {
    zTraceWarn("invalid input pMsg %p or addr %08x, @%s,%d.\n", pMsg, (pMsg ? pMsg->dstAddr : 0), function, line);
    return -1;
  }

  for(i=0; i<pMgr->busyRoute; i++)
  {
    zMsgRoute_t *route = &pMgr->listRoute[i];

    if(!route->addr) continue;
    
    if(route->fnCompare(route->addr, pMsg->dstAddr)) continue;

    void *qid = 0;
    if(route->queAddr.fnAddr)
    {
      qid = route->queAddr.fnAddr(pMsg->dstAddr);
    }
    
    if(!qid)
    {
      qid = route->queAddr.qid;
    }
    
    if(qid)
    {
      int ret = -1;
      if(pMsg->msgHandle > 0)      
        ret = zFastEnqueEx(qid, pMsg, sizeof(zMsg_t), function, line);
      else //non-standard local long msgs, e.g. timeout
        ret = zFastEnqueEx(qid, pMsg, sizeof(zMsgHdr_t), function, line);
        
      if(ret < 0)
      {
        zTraceWarn("failed to enqueue pMsg to addr: 0x%08x, @%s,%d..\n", pMsg->dstAddr, function, line);
        DEL_MSG(pMgr, pMsg->msgHandle);
        return ret;
      }

      return 0;
    }
  }

  //failed to find a route
  zTraceWarn("failed to enqueue pMsg to addr: 0x%08x, no route, @%s,%d..\n", pMsg->dstAddr, function, line);
  DEL_MSG(pMgr, pMsg->msgHandle);
  return -2;
}


int zMsgQue2Udp(void *serviceCore, void* qid, int sock, int *errNo, const char* function, int line)
{
  int rc;
 
  zMsgHdr_t* pMsg; 


  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore, @%s,%d.\n", function, line);
    return -1;
  }
 
  //ensure writable first
  rc = zCanWriteSocket(sock);
  if(rc < 0)
  {
    zTraceAlarm("Send socket error: sock=%d\n", sock);
    if(errNo) *errNo = errno;
    return rc;
  }
  if(rc == 0) return 0;
 
  zMsgHdr_t tempMsg;  
  rc = zMsgQueRecvEx(qid, &tempMsg, function, line);
  if(rc == 0) return 0; //no msgs
  
  if(rc != sizeof(zMsg_t))
  {
    zTraceAlarm("Send queue read error: qid=%p, msg size:%d\n", qid, rc);
    return rc;
  }

  zAddr_t srcAddr = tempMsg.srcAddr;
  zAddr_t dstAddr = tempMsg.dstAddr;
  zHandle_t msgHandle = tempMsg.msgHandle;
  
  if(msgHandle <= 0) //msgs to external must be standard zMsg.
  {
    zTraceError("invalid msg handle, msgHandle: %08x, from %08x to %08x\n", msgHandle, srcAddr, dstAddr);
    return -1;
  }
  
  int totalLen;
  pMsg = zMsgMapBuff(serviceCore, msgHandle, &totalLen);
  if(!pMsg)
  {
    zTraceError("invalid message handle, msgHandle: %08x, from %08x to %08x\n", msgHandle, srcAddr, dstAddr);
    return -1;
  }
  
  if(totalLen < pMsg->msgLen + sizeof(zMsgHdr_t)) //invalid pMsg length
  {
    zTraceError("invalid message length, msgHandle: %08x, from %08x to %08x\n", msgHandle, srcAddr, dstAddr);
    
    DEL_MSG(serviceCore, msgHandle);
    return -1;
  }
    
  int i;

  rc = -1;

  zTraceMinor("??Recv pMsg msgHandle: %08x,   from %08x to %08x   msgId: %04x msgLen: %d totalLen: %d\n", 
               msgHandle,                  srcAddr, dstAddr,   pMsg->msgId, pMsg->msgLen, totalLen);

  if(pMgr->hookQue2Udp)
  {
    zTraceInfo("msg hook: %s\n", symFuncNameGet(pMgr->hookQue2Udp));
    rc = pMgr->hookQue2Udp(pMsg);
    
    if(rc != 0) 
    {
      DEL_MSG(pMgr, msgHandle);
      return rc;
    }
  }
  dword_t dMsgLen = pMsg->msgLen + sizeof(zMsgHdr_t);
  for(i=0; i<pMgr->busyRoute; i++)
  {
    zMsgRoute_t *route = &pMgr->listRoute[i];

    dword_t ip = 0; 
    word_t port = 0;

    if(!route->addr) continue;
  
    if(route->fnCompare(route->addr, pMsg->dstAddr)) continue;

    if(route->nextHop.fnAddr)
    {
      route->nextHop.fnAddr(pMsg->dstAddr, &ip, &port);
    }
    if(!ip) ip = route->nextHop.ip;
    if(!port) port = route->nextHop.port;
    
    if(!ip || !port)
    {
      zTraceError("Invalid route target IP:%08x PORT:%d, msgHandle: %08x, from %08x to %08x\n", ip, port, msgHandle, srcAddr, dstAddr);
      break;
    }
    /*
     *whb add it. when sent to the internet, must always use the network byte order
     */
     zMsgHeadHdrHtoN(pMsg);
	/*
     *whb add it. when sent to the internet, must always use the network byte order
     */
    //rc = zSendto(sock, ip, port, (byte_t *)pMsg, pMsg->msgLen+sizeof(zMsgHdr_t));
    rc = zSendto(sock, ip, port, (byte_t *)pMsg, dMsgLen);
    if(rc < 0)
    {
      zTraceWarn("Failed to send pMsg, msgHandle: %08x, from %08x to %08x (IP:%08x PORT:%d)\n", msgHandle, srcAddr, dstAddr, ip, port);
      break;
    }
    
    break;
  }

  DEL_MSG(pMgr, msgHandle);
  
  return rc; //to tell the caller, it failed to send message because of no route.
}

int zMsgUdp2Que(void *serviceCore, int sock, int *errNo, const char* function, int line)
{
  int rc;
  
  zMsgHdr_t* pMsg;

  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore, @%s,%d.\n", function, line);
    return -1;
  }
  
  //ensure writable first
  rc = zCanReadSocket(sock);
  if(rc < 0)
  {
    zTraceAlarm("Recv socket error: sock=%d\n", sock);
    if(errNo) *errNo = errno;
    return rc;
  }
  if(rc == 0) return 0;
 
  zHandle_t msgHandle = NEW_MSG(pMgr, &pMsg);
  if(msgHandle <= 0)
  {
    zTraceAlarm("failed to alloc msgHandle, overflow.\n");
    return -1;
  }

  dword_t rip; word_t rport;
  rc = zRecvfrom(sock, &rip, &rport, (byte_t *)pMsg, 1024);
  if(rc > 0)
  { 
    pMsg->msgLen = rc - sizeof(zMsgHdr_t);
    pMsg->msgHandle = msgHandle;
    /*
     *whb add it. when we get a data from the network,we must chane it to host byte order
     */
     zMsgHeadHdrNtoH(pMsg);
     /*
     *whb add it. when we get a data from the network,we must chane it to host byte order
     */

    zTraceMinor("??Recv pMsg @%08x:%d, msgHandle: %08x, from %08x to %08x\n", rip, rport, pMsg->msgHandle, pMsg->srcAddr, pMsg->dstAddr);
    
    if(pMgr->hookUdp2Que)
    {
      zTraceMinor("msg hook: %s\n", symFuncNameGet(pMgr->hookUdp2Que));
      rc = pMgr->hookUdp2Que(pMsg, rip, rport);
      
      if(rc != 0)
      {
        DEL_MSG(pMgr, msgHandle);
        return rc;
      }
    }

    
    pMsg->dstAddr &= 0x00FFFFFF;
    rc = zMsgSend(pMgr, pMsg);
    if(rc < 0)
    {
      zTraceWarn("??Send pMsg from %08x to %08x failed.\n", pMsg->srcAddr, pMsg->dstAddr);

      DEL_MSG(pMgr, msgHandle);      
      pMsg = 0; 
      
      return -1;
    }
  }
  else
  {
    DEL_MSG(pMgr, msgHandle);    
    pMsg = 0;
  }
  
  return rc;
}


int zMsgBuffPoolShow(void* serviceCore, int flag)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  MsgPoolMgr_t *pPool = &pMgr->msgPool;

  zTraceP("MSG SHARE MEMORY\n");
  zTraceP("    KEY:        %08x\n", pPool->shmKey);
  zTraceP("    SHM TOTAL:  %d\n", pPool->shmTotal);
  zTraceP("    BUFFER:\n");
  zTraceP("      WIDTH:    %d\n", pPool->dataSize);
  zTraceP("      ID:       %08x\n", pPool->shmId);
  zTraceP("    BLOCK:\n");
  zTraceP("      COUNT:    %d\n", pPool->blkCount);
  zTraceP("      WIDTH:    %d\n", pPool->blkWidth);
  zTraceP("      NEXT:     %d\n", pPool->blkNext);
  zTraceP("      BLK ADDR: %p\n", pPool->blkTable);

  
  if(flag != 0)
  {
    int i; 
    int tNow = zTime();
    
    for(i=1; i<pPool->blkCount; i++)
    {
      BlkHdr_t *p = (BlkHdr_t *)(pPool->blkTable + pPool->blkWidth * i);
            
      if(flag < 2 && (tNow - p->tAlloc > MAX_MSG_CYCLE)) continue;

      zTraceP("[%4d] %s  -%-4d %s %d ::", i, p->used?"USED":"IDLE", MIN(tNow - p->tAlloc, MAX_MSG_CYCLE), p->used?p->function:"", p->line);
      zTraceHexP(p+1, 32);
      zTraceP("\n");
    }
  }
  
  return 0;
}



int zMsgShowRoute(void* serviceCore, zAddr_t addr)
{
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  int i;

  zTraceP("MAX  ROUTES:   %d\n", pMgr->maxRoute);
  zTraceP("BUSY ROUTES:   %d\n", pMgr->busyRoute);

  zTraceP("INDEX    ADDR   QUE      IP : PORT\n");
  for(i=0; i<pMgr->busyRoute; i++)
  {
    zMsgRoute_t *route = &pMgr->listRoute[i];

    //if(!route->addr) continue; //to print deleted routes
    
    if(addr && route->fnCompare(route->addr, addr)) continue;
      
    zTraceP("ROUTE %d :  addr %08x  %s", i, route->addr, symFuncNameGet(route->fnCompare));
    zTraceP("QueAddr: %p, %s", route->queAddr.qid, symFuncNameGet(route->queAddr.fnAddr));
    zTraceP("UDP ADDR %08x:%d, %s", route->nextHop.ip, route->nextHop.port, symFuncNameGet(route->nextHop.fnAddr));
    zTraceP("\n");
  }

  return 0;
}

int zMsgShow(void* serviceCore)
{
  if(!serviceCore) serviceCore = lastServiceCore;
  
  zMsgMgr_t* pMgr = (zMsgMgr_t *)serviceCore;  
  if(!pMgr)
  {
    zTraceError("invalid NULL serviceCore.\n");
    return -1;
  }

  zTraceP("SERVICE:  %p\n", pMgr);
  //zTraceP("SEM:      %x\n", pMgr->sem);

  zMsgBuffPoolShow(pMgr, 2);
  zMsgShowRoute(pMgr, 0);
  
  return 0;
}

#define MSG_SELFTEST

#ifdef MSG_SELFTEST

int testMsg()
{
  int ret;
  
  int shmKey = 123;
  int dataSize = 200;
  int buffCount = 20;
  
  void* serviceCore = zMsgServiceInit(4, 123, shmKey, dataSize, buffCount, 1);
  zMsgShow(serviceCore);

  zTraceP("\nTest Msg Buff New.\n");

  zMsgHdr_t *pMsg = 0;
  zMsgHdr_t *pMap = 0;
  int len = 0;

  zHandle_t msgHandle1 = 0;
  msgHandle1 = NEW_MSG(serviceCore, &pMsg);
  pMap = zMsgMapBuff(serviceCore, msgHandle1, &len);
  zTraceP("NEW: %08x   %p,  map %p, %d\n", msgHandle1, pMsg, pMap, len);
  
  zHandle_t msgHandle2 = 0;
  msgHandle2 = NEW_MSG(serviceCore, &pMsg);
  pMap = zMsgMapBuff(serviceCore, msgHandle2, &len);
  zTraceP("NEW: %08x   %p,  map %p, %d\n", msgHandle2, pMsg, pMap, len);

  zHandle_t msgHandle3 = 0;
  msgHandle3 = NEW_MSG(serviceCore, &pMsg);
  pMap = zMsgMapBuff(serviceCore, msgHandle3, &len);
  zTraceP("NEW: %08x   %p,  map %p, %d\n", msgHandle3, pMsg, pMap, len);

  zHandle_t msgHandle4 = 0;
  msgHandle4 = NEW_MSG(serviceCore, &pMsg);
  pMap = zMsgMapBuff(serviceCore, msgHandle4, &len);
  zTraceP("NEW: %08x   %p,  map %p, %d\n", msgHandle4, pMsg, pMap, len);
  zMsgShow(serviceCore);

  zTraceP("\nTest Msg Buff Free.\n");
  DEL_MSG(serviceCore, msgHandle1);  
  DEL_MSG(serviceCore, msgHandle2);
  DEL_MSG(serviceCore, msgHandle3);
  DEL_MSG(serviceCore, msgHandle4);
  zMsgShow(serviceCore);

  

  void* qid1 = zFastQueInit(TEST_QUE, 10, sizeof(zMsgHdr_t));
  zMsgAddRoute(serviceCore, 0x11, qid1, 0, 0);

  
  zMsgHdr_t tempMsg;
  tempMsg.msgHandle = 0;

  zTraceP("\n-----------------------------test queue 1--------------------\n");
  tempMsg.srcAddr = 1;  
  tempMsg.dstAddr = 0x11;
  ret = zMsgSend(serviceCore, &tempMsg);
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);

  zFastQueShow(qid1);

  ret = zFastDeque(qid1, &tempMsg, sizeof(tempMsg));
  zTraceP("src %08x dst %08x ret: %d\n", tempMsg.srcAddr, tempMsg.dstAddr, ret);
  
  zFastQueShow(qid1);
   

  return 0;
}

#endif

