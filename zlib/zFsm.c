/*----------------------------------------------------------
File Name  : xxx.c
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>

#include "cshell.h"

#include "zFsm.h"
#include "zFsmApi.h"
#include "zTraceApi.h"

static void* lastServiceCore = 0;
void* zFsmServiceInit(int maxTraceNum)
{
  void *serviceCore = zFsmTraceInit(maxTraceNum);
  if(!serviceCore)
  {
    zTraceFatal("fsm trace init pool failed, num=%d.\n", maxTraceNum);

    return 0;
  }
  lastServiceCore = serviceCore;

  return serviceCore;
}

LOCAL int zFsmBlockCompare(const void *a, const void *b)
{
  zFsmBlock_t *p0 = (zFsmBlock_t*)a;
  zFsmBlock_t *p1 = (zFsmBlock_t*)b;

  return (p0->wStateIn == p1->wStateIn) ?
              (p1->wEventIn - p0->wEventIn) : (p1->wStateIn - p0->wStateIn);
}

LOCAL zFsmBlock_t *zFsmBlockGet(zFsmBlock_t *pTable, word_t maxNum, word_t wEvent, word_t wState)
{
  void *pRet = 0;

  zFsmBlock_t tmpBlock = {wState, wEvent, };
  pRet = bsearch(&tmpBlock, pTable, maxNum, sizeof(zFsmBlock_t), zFsmBlockCompare);

  if(!pRet)
  {
    tmpBlock.wStateIn = eStateEventAny;
    pRet = bsearch(&tmpBlock, pTable, maxNum, sizeof(zFsmBlock_t), zFsmBlockCompare);

    if(!pRet)
    {
      tmpBlock.wEventIn = eStateEventAny;
      pRet = bsearch(&tmpBlock, pTable, maxNum, sizeof(zFsmBlock_t), zFsmBlockCompare);
    }
  }

  return (zFsmBlock_t *)pRet;
}

int zFsmTableFormat(zFsmBlock_t *pTable, word_t maxNum)
{
  qsort(pTable, maxNum, sizeof(zFsmBlock_t), zFsmBlockCompare);

  return 0;
}

int zFsmTableShow(zFsmPanel_t *pPanel)
{
  int i, j;

  if(!pPanel)
  {
    zTraceP("invalid input null panel.\n");
    return -1;
  }

  word_t maxNum = 0;
  if(!pPanel->fnTableGet)
  {
    zTraceP("invalid input null fnTableGet.\n");
    return -1;
  }
  zFsmBlock_t *pTable = pPanel->fnTableGet(&maxNum);
  if(!pTable || !maxNum)
  {
    zTraceP("invalid input null table.\n");
    return -1;
  }

  zTraceP("Table Size = %d\n", maxNum);


  zTraceP("      CURRENT STATE                      ON EVENT                      NEW STATE        --   Exception  Done          FUNCTIIONS \n");
  for(i=0; i<maxNum; i++)
  {
    zTraceP("%2d -- %25s[%3d] %30s[%3d]  %30s[%3d] ", i,
                               pPanel->fnStateName(pTable[i].wStateIn), pTable[i].wStateIn,
                               pPanel->fnEventName(pTable[i].wEventIn), pTable[i].wEventIn,
                               pPanel->fnStateName(pTable[i].wStateOut), pTable[i].wStateOut);
    zTraceP(" <%8d>  <%8d> ", pTable[i].statistic.exception, pTable[i].statistic.done);
    for(j=0; j<Z_FSM_FUNC_LIST_SIZE; j++)
    {
      if(!pTable[i].fnCall[j]) break;

      zTraceP(" %s <%d>,", symFuncNameGet(pTable[i].fnCall[j]), pTable[i].statistic.transfer[j]);
    }
    
    zTraceP("\n");
  }

  return 0;
}

int zFsmTableStatReset(zFsmPanel_t *pPanel)
{
  if(!pPanel)
  {
    zTraceP("invalid input null panel.\n");
    return -1;
  }

  word_t maxNum = 0;
  if(!pPanel->fnTableGet)
  {
    zTraceP("invalid input null fnTableGet.\n");
    return -1;
  }
  zFsmBlock_t *pTable = pPanel->fnTableGet(&maxNum);
  if(!pTable || !maxNum)
  {
    zTraceP("invalid input null table.\n");
    return -1;
  }

  int i;
  for(i=0; i<maxNum; i++)
  {
    memset(&pTable[i].statistic, 0, sizeof(zFsmStat_t));
  }

  return 0;
}


int zFsmPanelShow(zFsmPanel_t *pPanel)
{
  if(!pPanel)
  {
    return -1;
  }

  zTraceP("fnStateGet    = %p %s\n", pPanel->fnStateGet, symFuncNameGet(pPanel->fnStateGet));
  zTraceP("fnStateSet    = %p %s\n", pPanel->fnStateSet, symFuncNameGet(pPanel->fnStateSet));

  zTraceP("fnInstGet     = %p %s\n", pPanel->fnInstGet,  symFuncNameGet(pPanel->fnInstGet));
  //zTraceP("fnEventGet    = %p %s\n", pPanel->fnEventGet, symFuncNameGet(pPanel->fnEventGet));

  zTraceP("fnTraceGet    = %p %s\n", pPanel->fnTraceGet, symFuncNameGet(pPanel->fnTraceGet));
  zTraceP("fnStateName   = %p %s\n", pPanel->fnStateName, symFuncNameGet(pPanel->fnStateName));
  zTraceP("fnEventName   = %p %s\n", pPanel->fnEventName, symFuncNameGet(pPanel->fnEventName));


  zTraceP("fnFsmTableGet = %p %s\n", pPanel->fnTableGet, symFuncNameGet(pPanel->fnTableGet));


  return 0;
}



int zFsmAllocTraceId()
{
  static word_t nextTraceId = 1;

  if(nextTraceId <= 0) nextTraceId = 1;

  return nextTraceId++;
}

int zFsmProcessMsg(const void* serviceCore, const zFsmPanel_t *pPanel, word_t wEvent, int nInst, zMsgHdr_t *pMsg, zFsmContext_t *pContext)
{
  void*    pInst = 0;

  word_t   wState = eStateEventAny;

  int      traceId = -1;
  int      traceIndex = -1;

  int      fsmDepth = 0;

  int      objectID;


  word_t      sizeTable;
  zFsmBlock_t   *pFsmTable;
  zFsmBlock_t   *pFsmBlock;


  int index;
  int eventRet;

  if(!pPanel)
  {
    zTraceError("invalid input null pPanel=%p\n", pPanel);
    return -1;
  }
  
  pInst = pPanel->fnInstGet(wEvent, nInst, pMsg, &objectID);
  if(!pInst)
  {
    zTraceError("can't find instance %d\n", nInst);
    return -1;
  }

  wState = pPanel->fnStateGet(pInst);
  traceId = pPanel->fnTraceGet(pInst, pMsg);
  if(traceId > 0)
  {
    traceIndex = zFsmTraceNew(serviceCore, pPanel, traceId);
  }

  zFsmTraceUpdateInput(serviceCore, traceIndex, objectID, wEvent, wState, pMsg);

  pFsmTable = pPanel->fnTableGet(&sizeTable);
  if(!pFsmTable)
  {
    zTraceWarn("Get pFsmTable failed:%d\n", wEvent);
    return wEvent;
  }

  /* FSM loop begins */
  event_proc:

  zTraceDebug("-------event %s state %s\n", pPanel->fnEventName(wEvent), pPanel->fnStateName(pPanel->fnStateGet(pInst)));

  fsmDepth += 1;

  if(fsmDepth > Z_FSM_DEPTH_MAX)
  {
    zTraceError("invalid infinite fsm loop: EVENT=%s, TraceID=%x\n", pPanel->fnEventName(wEvent), traceId);
    return -1;
  }

  pFsmBlock = zFsmBlockGet(pFsmTable, sizeTable, wEvent, wState);
  if(!pFsmBlock)
  {
    //zTraceWarn("Get FsmBlock failed, event %d, state: %d\n", wEvent, wState);
    return wEvent;
  }

  eventRet = eStateEventAny;
  zFsmFunCall_t pCall = 0;
  for(index=0; index<Z_FSM_FUNC_LIST_SIZE; index++)
  {
    pCall = (zFsmFunCall_t)pFsmBlock->fnCall[index];

    if(!pCall) break;

    //zTraceDebug("#######Before pCall %s on %s\n", symFuncNameGet(pCall), pPanel->fnEventName(wEvent));

    zFsmTraceUpdateFunCall(serviceCore, traceIndex, pCall, index);

    //clean previous eos
    pContext->errstr = 0;
    pContext->user1 = 0;
    pContext->user2 = 0;
    
    eventRet = pCall(wEvent, pInst, pMsg, pContext);

    if(pContext->errstr) zFsmTraceUpdateErrstr(serviceCore, traceIndex, pContext->errstr, pContext->user1, pContext->user2);
    

    //zTraceDebug("-------After pCall %s on %s\n", symFuncNameGet(pCall), pPanel->fnEventName(wEvent));

    if(eventRet != eStateEventAny)  break;
  }

  if(eventRet == eStateEventAny)
  {
    pFsmBlock->statistic.done ++;
    
    zFsmTraceUpdateOutput(serviceCore, traceIndex, eStateEventAny, pFsmBlock->wStateOut, 0);

    if(pFsmBlock->wStateOut != eStateEventAny) 
    {
      pPanel->fnStateSet(pInst, pFsmBlock->wStateOut);
      wState = pFsmBlock->wStateOut;
    }

    return 0;
  }
  else
  if(eventRet > eStateEventAny)
  {
    pFsmBlock->statistic.transfer[index] ++;
    
    /* reach to the last fun call */
    if((index >= Z_FSM_FUNC_LIST_SIZE - 1) || !(pFsmBlock->fnCall[index+1]))
    {
      zFsmTraceUpdateOutput(serviceCore, traceIndex, eventRet, pFsmBlock->wStateOut, 0);

      if(pFsmBlock->wStateOut != eStateEventAny)
      {
        pPanel->fnStateSet(pInst, pFsmBlock->wStateOut);
        wState = pFsmBlock->wStateOut;
      }

      wEvent = eventRet;

      traceIndex = zFsmTraceNew(serviceCore, pPanel, traceId);
      zFsmTraceUpdateInput(serviceCore, traceIndex, objectID, wEvent, wState, 0);

      goto event_proc;
    }
    else
    {
      zFsmTraceUpdateOutput(serviceCore, traceIndex, eventRet, eStateEventAny, 0);

      wEvent = eventRet;

      traceIndex = zFsmTraceNew(serviceCore, pPanel, traceId);
      zFsmTraceUpdateInput(serviceCore, traceIndex, objectID, wEvent, wState, 0);

      goto event_proc;
    }
  }
  else
  /*if(eventRet < eStateEventAny)*/
  {
    pFsmBlock->statistic.exception ++;
    
    zTraceError("failed to continue due to exception: %d, pCall: %p %s\n", eventRet, pCall, symFuncNameGet(pCall));

    zFsmTraceUpdateOutput(serviceCore, traceIndex, eStateEventAny, eStateEventAny, eventRet);

    return -2;
  }
}


int zFsmCacheEnqueue(zFsmContext_t* pContext, word_t wEvent, zMsgHdr_t *pMsg, dword_t dwUser)
{
  int next;
  zFsmCacheQueue_t *pCacheQueue = (zFsmCacheQueue_t*)pContext;

  if(!pCacheQueue)
  {
    zTraceWarn("Cache Queue isn't availabel.\n");
    return -1;
  }

  next = (pCacheQueue->rear+1)%Z_FSM_CACHE_MAX;

  if(next == pCacheQueue->head)
  {
    zTraceWarn("Cache Queue overflow.\n");
    return -2;
  }

  pCacheQueue->cache[pCacheQueue->rear].wEvent = wEvent;
  pCacheQueue->cache[pCacheQueue->rear].pMsg = pMsg;
  pCacheQueue->cache[pCacheQueue->rear].dwUser = dwUser;

  pCacheQueue->rear = next;

  return 0;
}

int zFsmCacheDequeue(zFsmContext_t* pContext, word_t *pwEvent, zMsgHdr_t** ppMsg, dword_t *pdwUser)
{
  zFsmCacheQueue_t *pCacheQueue = (zFsmCacheQueue_t*)pContext;

  if(!pCacheQueue)
  {
    zTraceWarn("Cache Queue isn't availabel.\n");
    return -1;
  }

  if(pCacheQueue->rear == pCacheQueue->head)
  {
    return 0;
  }

  if(pwEvent) *pwEvent = pCacheQueue->cache[pCacheQueue->head].wEvent;
  if(ppMsg)   *ppMsg = pCacheQueue->cache[pCacheQueue->head].pMsg;
  if(pdwUser) *pdwUser = pCacheQueue->cache[pCacheQueue->head].dwUser;

  pCacheQueue->head = (pCacheQueue->head+1) % Z_FSM_CACHE_MAX;

  return 1;
}


int zFsmProcessMsgEx(const void* serviceCore, const zFsmPanel_t *pPanel, word_t wEventIn, int nInstIn, zMsgHdr_t* pMsgIn)
{
  int iRet;

  zFsmCacheQueue_t cacheQueue;

  cacheQueue.head = 0;
  cacheQueue.rear = 0;

  word_t wEvent = wEventIn;
  zMsgHdr_t *pMsg = pMsgIn;
  int nInst = nInstIn;

  iRet = zFsmProcessMsg(serviceCore, pPanel, wEvent, nInst, pMsg, &cacheQueue.context);
  if(iRet == wEvent)
  {
    zTraceWarn("No rule for EVENT=%s\n", pPanel->fnEventName(wEvent));
  }

  do
  {
    int iRet2 = zFsmCacheDequeue(&cacheQueue.context, &wEvent, &pMsg, (dword_t*)&nInst);
    if(iRet2 <= 0) break;

    iRet = zFsmProcessMsg(serviceCore, pPanel, wEvent, nInst, pMsg, &cacheQueue.context);
    if(iRet == wEvent)
    {
      zTraceWarn("No rule for EVENT=%s\n", pPanel->fnEventName(wEvent));
    }
  }while(iRet >= 0);

  return iRet;
}


word_t zMsg2Event(word_t msg, zMsg2EventMap_t arr[], int nSize)
{
  int i;

  for(i=0; i<nSize; i++)
  {
    if(arr[i].msg == msg) return arr[i].event;
  }

  return 0;
}


