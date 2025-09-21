/*----------------------------------------------------------
File Name  : xxx.c
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"

#include "cshell.h"

#include "zFsm.h"
#include "zFsmApi.h"
#include "zTraceApi.h"

#include <time.h>

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <string.h>

extern char *strcasestr(const char *haystack, const char *needle);

int zFmsTraceReset(const void *serviceCore)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore);
  if(!pTable) return -1;

  {
    int maxTraceNum = pTable->maxNum;

    memset(pTable->trace, 0, sizeof(pTable->trace[0])* pTable->maxNum);

    pTable->maxNum = maxTraceNum;
    pTable->nextFree = 0;
  }

  return 0;
}


void* zFsmTraceInit(int maxTraceNum)
{
  zFsmTraceTable_t *pTable;

  int size;

  maxTraceNum = maxTraceNum>0 ? maxTraceNum : 1024;

  size = (byte_t*)&pTable->trace[0] - (byte_t*)pTable;

  size += sizeof(pTable->trace[0]) * maxTraceNum;

  pTable = (zFsmTraceTable_t *)malloc(size);
  if(!pTable)
  {
    zTraceFatal("failed to alloc fsm trace table, size=%d\n", size);

    return 0;
  }

  memset(pTable, 0, size);

  pTable->maxNum = maxTraceNum;

  return pTable;
}


int zFsmTraceNew(const void* serviceCore, const zFsmPanel_t *pPanel, int traceId)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore);
  zFsmTrace_t *pTrace;

  int traceIndex;

  if(traceId < 0) return -1;
  
  if(pTable->nextFree >= pTable->maxNum) pTable->nextFree = 0;

  traceIndex = pTable->nextFree;

  pTrace = &pTable->trace[traceIndex];

  memset(pTrace, 0, sizeof(*pTrace));

  pTrace->timeCreate = time(0);
  pTrace->pPanel = pPanel;
  pTrace->traceId = traceId;

  pTable->nextFree += 1;

  return traceIndex;
}



int zFsmTraceUpdateInput(const void *serviceCore, int traceIndex, int nObjID, word_t wEvent, word_t wState, zMsgHdr_t *pMsg)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore);
  zFsmTrace_t *pTrace;

  if(traceIndex < 0 || traceIndex >= pTable->maxNum)
  {
    return 0;
  }

  /* we don't lock here, since we suppose the pool is larger enough*/
  pTrace = &pTable->trace[traceIndex];

  if(pMsg)
  {
    pTrace->stMsg = *pMsg;
  }

  pTrace->nObjID = nObjID;
  pTrace->wEventIn = wEvent;
  pTrace->wStateIn = wState;

  return 1;
}

int zFsmTraceUpdateFunCall(const void *serviceCore, int traceIndex, zFsmFunCall_t fnCall, int nIndex)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore);
  zFsmTrace_t *pTrace;

  if(traceIndex < 0 || traceIndex >= pTable->maxNum)
  {
    return 0;
  }

  /* we don't lock here, since we suppose the pool is larger enough*/
  pTrace = &pTable->trace[traceIndex];

  pTrace->fnCall[nIndex] = fnCall;

  return 1;
}

int zFsmTraceUpdateOutput(const void* serviceCore, int traceIndex, word_t wEvent, word_t wState, int exception)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore);
  zFsmTrace_t *pTrace;

  if(traceIndex < 0 || traceIndex >= pTable->maxNum)
  {
    return 0;
  }

  /* we don't lock here, since we suppose the pool is larger enough*/
  pTrace = &pTable->trace[traceIndex];

  pTrace->wEventOut = wEvent;
  pTrace->wStateOut = wState;
  pTrace->exception = exception;

  pTrace->timeDone = time(0);

  return 1;
}



int zFsmTraceShow(const void* serviceCore, int traceId, int bEosFlag)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore); 
  if(!pTable) return -1;

  int i;

  for(i=0; i<pTable->maxNum; i++)
  {
    zFsmTrace_t *pTrace;
    const zFsmPanel_t *pPanel;
    const void      *pMsg;

    int traceIndex = pTable->nextFree + i;

    traceIndex %= pTable->maxNum;

    pTrace = &pTable->trace[traceIndex];
    if(pTrace->traceId == 0) continue;

    /* Since we don't lock the pool, there's risk that ptr is set to 0, so copy them out... */
    pPanel = pTrace->pPanel;
    pMsg   = &pTrace->stMsg;

    if(!pMsg || !pPanel) continue;

    if((traceId > 0) && (traceId != pTrace->traceId)) continue;
    if(bEosFlag && !pTrace->eos[0].errstr) continue;


    zTraceP("[%4d] Panel:%p ", traceIndex, pPanel);
    zTraceP("Start@%d ", (pTrace->timeCreate));
    zTraceP("Stop@%d ", (pTrace->timeDone));
    zTraceP("Trace: %x ", pTrace->traceId);
    zTraceP("\n");
    pPanel->fnMsgPrint(&pTrace->stMsg);


    if(pTrace->nObjID)
      zTraceP("      [ID: %d]", pTrace->nObjID);
    else
      zTraceP("             ");
      
    zTraceP("%d %s + %d %s\n", pTrace->wStateIn, pPanel->fnStateName(pTrace->wStateIn),
                                   pTrace->wEventIn, pPanel->fnEventName(pTrace->wEventIn));
    zTraceP("              ==>> ");
    {
      int j;

      for(j=0; j<Z_FSM_FUNC_LIST_SIZE; j++)
      {
        if(!pTrace->fnCall[j]) break;

        zTraceP("%s, ", symFuncNameGet(pTrace->fnCall[j]));
      }
    }
    zTraceP("\n");

    zTraceP("                 ");
    zTraceP("= %d %s,  %d %s\n", pTrace->wStateOut, pPanel->fnStateName(pTrace->wStateOut),
                                     pTrace->wEventOut, pPanel->fnEventName(pTrace->wEventOut));
    {
      int j;
      for(j=0; j<MAX_ERRSTR_NUM; j++)
      {        
        if(!pTrace->eos[j].errstr) break;
        
        zTraceP("            **EOS %d**: %s (USER1 %08x USER2 %08x)\n", j, pTrace->eos[j].errstr, pTrace->eos[j].user1, pTrace->eos[j].user2);
      }
    }
  }

  return 0;
}


int zFsmTraceUpdateErrstr(const void* serviceCore, int traceIndex, const char* errstr, dword_t user1, dword_t user2)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore);
  zFsmTrace_t *pTrace;

  if(traceIndex < 0 || traceIndex >= pTable->maxNum)
  {
    return 0;
  }

  /* we don't lock here, since we suppose the pool is larger enough*/
  pTrace = &pTable->trace[traceIndex];

  int i;
  for(i=0; i<MAX_ERRSTR_NUM; i++)
  {
    if(!pTrace->eos[i].errstr)
    {      
      pTrace->eos[i].errstr = errstr;
      pTrace->eos[i].user1 = user1;
      pTrace->eos[i].user2 = user2;
      
      return 1;
    }
  }

  return 0;
}

int zFsmTraceReset(const void* serviceCore)
{
  zFsmTraceTable_t *pTable = zFsmTraceTablePtr(serviceCore); 
  if(!pTable) return -1;

  int i;

  for(i=0; i<pTable->maxNum; i++)
  {
    zFsmTrace_t *pTrace;

    pTrace = &pTable->trace[i];
    pTrace->traceId = 0;
  }

  zTraceP("maxNum: %d, next:%d. Reset next to 0.\n", pTable->maxNum, pTable->nextFree);
  pTable->nextFree = 0;

  return 0;
}

