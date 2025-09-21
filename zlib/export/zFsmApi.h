/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/



#ifndef __Z_FSM_API_H__
#define __Z_FSM_API_H__

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif



/*Below definition can't be changed. Otherwise, ...*/
#define eStateEventAny  0
#define eStateEventNull eStateEventAny


/*Change them per application logic*/
#define Z_FSM_DEPTH_MAX  32

#define Z_FSM_FUNC_LIST_SIZE 8

typedef struct _Z_FSM_CONTEXT_TYPE_
{
  void *pInstdfd;
  zMsgHdr_t *pMsgdfa;

  //eos mark
  const char* errstr;
  dword_t user1;
  dword_t user2;
} zFsmContext_t;


typedef struct _Z_FSM_STAT_TYPE_
{
  dword_t exception;
  dword_t done;
  dword_t transfer[Z_FSM_FUNC_LIST_SIZE];
} zFsmStat_t;


typedef int    (*zFsmFunCall_t)(word_t wEvent, void* pInst, zMsgHdr_t* pMsg, zFsmContext_t* pContext);
typedef struct _Z_FSM_BLOCK_TYPE_
{
  word_t  wStateIn;
  word_t  wEventIn;
  void* /*zFsmFunCall_t*/ fnCall[Z_FSM_FUNC_LIST_SIZE];
  word_t  wStateOut;

  zFsmStat_t statistic;
} zFsmBlock_t;





typedef word_t (*zFsmEventGet_t)(zMsgHdr_t *pMsg);
typedef void*  (*zFsmInstGet_t )(word_t wEvent, int nInst, zMsgHdr_t *pMsg, int *pOutID);

typedef word_t (*zFsmStateGet_t)(void *pInst);
typedef int    (*zFsmStateSet_t)(void *pInst, word_t wState);

typedef const char*  (*zFsmStateName_t)(word_t wState);
typedef const char*  (*zFsmEventName_t)(word_t wEvent);

typedef zFsmBlock_t* (*zFsmTableGet_t )(word_t *pMaxNum);

typedef int  (*zFsmMsgLenGet_t  )(zMsgHdr_t *pMsg);
typedef int  (*zFsmMsgPrint_t  )(zMsgHdr_t *pMsg);

typedef int    (*zFsmTraceGet_t)(void *pInst, zMsgHdr_t *pMsg);


typedef struct _Z_FSM_PANEL_TYPE_
{
  //zFsmEventGet_t  fnEventGet;
  zFsmInstGet_t   fnInstGet;
  zFsmTraceGet_t  fnTraceGet;
  zFsmStateName_t fnStateName;
  zFsmEventName_t fnEventName;

  zFsmStateGet_t  fnStateGet;
  zFsmStateSet_t  fnStateSet;
  zFsmTableGet_t  fnTableGet;

  zFsmMsgPrint_t  fnMsgPrint;
} zFsmPanel_t;

void* zFsmServiceInit(int maxTraceNum);
int zFsmTableFormat(zFsmBlock_t *pTable, word_t maxNum);
int zFsmTableShow(zFsmPanel_t *pPanel);
int zFsmPanelShow(zFsmPanel_t *pPanel);
int zFsmTableStatReset(zFsmPanel_t *pPanel);


int zFsmAllocTraceId();

int zFsmProcessMsg(const void* serviceCore, const zFsmPanel_t *pPanel, word_t wEvent, int nInst, zMsgHdr_t* pMsg, zFsmContext_t* pContext);


typedef struct MSG2EVENT_MAP_T
{
  word_t msg;
  word_t event;
} zMsg2EventMap_t;

word_t zMsg2Event(word_t msg, zMsg2EventMap_t arr[], int nSize);


#define Z_FSM_CACHE_MAX 16
typedef struct Z_CONTEXT_CACHE_EVENT_TYPE_
{
  word_t  wEvent;
  word_t  wRsvd;
  dword_t dwUser;
  zMsgHdr_t  *pMsg;
} PACKED zFsmCacheEvent_t;

typedef struct _Z_FSM_CACHE_QUEUE_TYPE
{
  zFsmContext_t context;

  int rear;
  int head;
  zFsmCacheEvent_t cache[Z_FSM_CACHE_MAX];
} zFsmCacheQueue_t;

int zFsmCacheDequeue(zFsmContext_t* pContext, word_t *pwEvent, zMsgHdr_t** ppMsg, dword_t *pdwUser);
int zFsmCacheEnqueue(zFsmContext_t* pContext, word_t wEvent, zMsgHdr_t* pMsg, dword_t dwUser);

int zFsmProcessMsgEx(const void* serviceCore, const zFsmPanel_t *pPanel, word_t wEventIn, int nInstIn, zMsgHdr_t* pMsgIn);
int zFsmTraceShow(const void* serviceCore, int traceId, int bEosFlag);

int zFsmTraceUpdateErrstr(const void *serviceCore, int traceIndex, const char* errstr, dword_t user1, dword_t user2);

#define FSM_TRACE_UPDATE_EOS(pContext, eos, user_data1, user_data2) do { \
  pContext->errstr = _STR(eos); \
  pContext->user1 = user_data1; \
  pContext->user2 = user_data2; \
}while(0)

#ifndef FSM_EOS_SET
#define FSM_EOS_SET(pContext, eos, param1, param2) do{ \
  EOS_SET_WP(eos, param1, param2); \
  FSM_TRACE_UPDATE_EOS(pContext, eos, param1, param2); \
}while(0)
#endif

int zFsmTraceReset(const void* serviceCore);

#ifdef __cplusplus
}
#endif

#endif /*__Z_FSM_API_H__*/






