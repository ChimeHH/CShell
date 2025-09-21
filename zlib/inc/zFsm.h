/*----------------------------------------------------------
File Name  : xxx.h
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_FSM_H__
#define __Z_FSM_H__



#include "zFsmApi.h"


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_ERRSTR_NUM 4
typedef struct _Z_FSM_EOS_TYPE_
{
  const char* errstr;
  dword_t     user1;
  dword_t     user2;
} zFsmEos_t;

typedef struct _Z_FSM_TRACE_TYPE_
{
  dword_t      traceId:30;
  dword_t      bExternal:1;

  int          timeCreate;
  int          timeDone;

  const zFsmPanel_t *pPanel;

  zMsgHdr_t    stMsg;

  int          nObjID;
  word_t       wEventIn;
  word_t       wStateIn;

  zFsmFunCall_t fnCall[Z_FSM_FUNC_LIST_SIZE];

  word_t       wEventOut;
  word_t       wStateOut;

  int          exception;

  zFsmEos_t    eos[MAX_ERRSTR_NUM];
} zFsmTrace_t;





typedef struct _Z_FSM_TRACE_TABLE_
{
  int    maxNum;
  int    nextFree;

  zFsmTrace_t trace[1];
} zFsmTraceTable_t;


int zFmsTraceReset(const void *serviceCore);
void* zFsmTraceInit(int maxTraceNum);

#define zFsmTraceTablePtr(serviceCore) ((zFsmTraceTable_t *)(serviceCore))

int zFsmTraceNew(const void* serviceCore, const zFsmPanel_t *pPanel, int traceId);

int zFsmTraceUpdateInput(const void *serviceCore, int traceIndex, int nObjID, word_t wEvent, word_t wState, zMsgHdr_t *pMsg);
int zFsmTraceUpdateFunCall(const void *serviceCore, int traceIndex, zFsmFunCall_t fnCall, int nIndex);
int zFsmTraceUpdateOutput(const void *serviceCore, int traceIndex, word_t wEvent, word_t wState, int eos);



#ifdef __cplusplus
}
#endif



#endif /*__Z_FSM_H__*/






