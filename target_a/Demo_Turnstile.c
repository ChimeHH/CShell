/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#define _GNU_SOURCE         /* See feature_test_macros(7) */
	
#include <string.h>
       

#include "zType_Def.h"
#include "zSalOS.h"


#include "zTraceApi.h"
#include "zFsmApi.h"

/*
A turnstile
An example of a very simple mechanism that can be modeled by a state machine
is a turnstile.[2][3] A turnstile, used to control access to subways and 
amusement park rides, is a gate with three rotating arms at waist height, 
one across the entryway. Initially the arms are locked, barring the entry, 
preventing customers from passing through. Depositing a coin or token in 
a slot on the turnstile unlocks the arms, allowing them to rotate by 
one-third of a complete turn, allowing a single customer to push through. 
After the customer passes through, the arms are locked again until another 
coin is inserted.

The turnstile has two states: Locked and Unlocked.[2] There are two inputs
that affect its state: putting a coin in the slot (coin) and pushing the arm
(push). In the locked state, pushing on the arm has no effect; no matter how 
many times the input push is given it stays in the locked state. Putting a 
coin in, that is giving the machine a coin input, shifts the state from Locked
to Unlocked. In the unlocked state, putting additional coins in has no effect; 
that is, giving additional coin inputs does not change the state. However, a 
customer pushing through the arms, giving a push input, shifts the state back 
to Locked.

The turnstile state machine can be represented by a state transition table, 
showing for each state the new state and the output (action) resulting from 
each input

Current State    Input	     Next State	      Output
Locked	         coin	     Unlocked	      Release turnstile so customer can push through
                 push	     Locked	          None
Unlocked	       coin	     Unlocked         None
                 push	     Locked	          When customer has pushed through lock turnstile
*/

//define a turnstile list
typedef struct TurnstileInstanceType
{
  word_t wState;

  int id;
  char *desc;
  
  int traceId;
} turnstile_t;

LOCAL turnstile_t s_turnstileList[4];

//define msg, event and state for turnstile
enum TurnstileMsgEnum
{  
  eMsgCoin = 1,
  eMsgPush,
  eMsgAttack,
  eMsgAudit,
  eMsgFixed,
  eMsgRepair,
};
enum TurnstileEventEnum
{  
  eEventCoin = 1,
  eEventPush,
  eEventAttack,
  eEventAudit,
  eEventFixed,
  eEventRepair,
  eEventOos,
  eEventUnknown,
};
enum TurnstileStateEnum
{
  eStateLocked = 1,
  eStateUnlocked,
  eStateOos,
  eStateUnknown,
};

const char* turnstileStateName(word_t wState)
{
  switch(wState)
  {
  case eStateLocked:   return "locked";
  case eStateUnlocked: return "unlocked";
  case eStateOos:      return "oos";
  default:;
  }

  return "*NA*";
}

const char* turnstileEventEnum2Name(word_t wEvent)
{
  switch(wEvent)
  {
  case eEventCoin:   return "coin";
  case eEventPush:   return "push";
  case eEventAttack:   return "hack";
  case eEventAudit:  return "audit";
  case eEventOos:    return "oos";
  case eEventFixed:  return "fixed";
  case eEventRepair: return "repair";
  default:;
  }

  return "*NA*";
}

const char* turnstileMsgEnum2Name(word_t msgId)
{
  switch(msgId)
  {
  case eMsgCoin:   return "Coin";
  case eMsgPush:   return "Push";
  case eMsgAttack: return "Attack";
  case eMsgAudit:  return "Audit";
  case eMsgFixed:  return "Fixed";
  case eMsgRepair: return "Repair";
  default:    ;
  }
  return "*NA*";
}

//Translate msg to event. Actually we have some better way to optimize these.
//First, we can have a sorted table, then use binary search, that would be must faster;
//Or we can have different value regions for messages and events. For example, if rule 
//says messages must be 0x0001~0x3fff, and then we can have external events respectively (use msg as event),
//and other values for internal events. In that case, we don't need a table to map message to event.
word_t turnstileEventGet(zMsgHdr_t *pMsg)
{
  switch(pMsg->msgId)
  {
    case eMsgCoin: return eEventCoin;
    case eMsgPush: return eEventPush;
    case eMsgAttack: return eEventAttack;
    case eMsgAudit:  return eEventAudit;
    case eMsgFixed:  return eEventFixed;
    case eMsgRepair: return eEventRepair;
    default:;
  }
  return eEventUnknown;
}


//define fsm schedule table. 
int GoodCoin(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int GoodPush(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int BadCoin(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int BadPush(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int OOS(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int Attack(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int Audit(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int Fixed(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);
int Repair(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext);

LOCAL zFsmBlock_t s_turnstileFsmBlks[] =
{
  {eStateLocked,   eEventCoin,  {GoodCoin, }, eStateUnlocked},
  {eStateLocked,   eEventPush,  {BadPush,  }, eStateLocked},
  {eStateUnlocked, eEventCoin,  {BadCoin,  }, eStateUnlocked},
  {eStateUnlocked, eEventPush,  {GoodPush, }, eStateLocked},

  {eStateLocked,   eEventAudit, {Audit,    }, eStateLocked},
  {eStateUnlocked, eEventAudit, {Audit,    }, eStateUnlocked},
  {eStateOos,      eEventFixed, {Fixed,    }, eStateLocked},
  {eStateOos,      0,           {Repair,   }, eStateLocked},

  {0,              eEventAttack,{Attack,   }, 0},
  
  {0,              eEventAudit, {Audit,    }, eStateLocked},
  {0,              eEventOos,   {OOS,      }, eStateOos},
  
  {0,              0,           {OOS,      }, eStateOos},
};

//Note, sometimes an instance might have multi sub states, hence there could be more than 1 fsm table.
//In that case, each of them could be defined as a panel, and programmer must define a rule for fsm to pick up a proper panel then.
//but here, we only define 1 panel.
LOCAL zFsmPanel_t s_turnstileFsmPanel;

//must define a service core for turnstile fsm, because we need some buffer to do trace.
void *s_turnstileFsmServiceCore = 0;

void*  turnstileInstanceGet(word_t wEvent, int nInst, zMsgHdr_t *pMsg, int *pOutID)
{
  if(nInst <= 0 || nInst >= TBL_SIZE(s_turnstileList)) nInst = pMsg->dstInst;
  if(nInst >= TBL_SIZE(s_turnstileList))
  {
    zTraceError("invalid input dstInst:%d\n", pMsg->dstInst);
    return 0;
  }
  
  return &s_turnstileList[nInst]; 
}

int turnstileFsmTraceGet(void* pInst, zMsgHdr_t *pMsg)
{
  static int traceId = 1;

  if(((turnstile_t *)pInst)->traceId == 0)
  {
    ((turnstile_t *)pInst)->traceId = traceId ++;
  }
  
  return ((turnstile_t *)pInst)->traceId;
}


int turnstileMsgPrint(zMsgHdr_t* pMsg)
{
  zTraceP("      MSG ID: %d %s   DEST INST: %d\n", pMsg->msgId, turnstileMsgEnum2Name(pMsg->msgId), pMsg->dstInst);

  return 0;
}


word_t turnstileStateGet(void *pInst)
{
  return ((turnstile_t *)pInst)->wState;
}

int turnstileStateSet(void *pInst, word_t wState)
{
  ((turnstile_t *)pInst)->wState = wState;

  return 0;
}

zFsmBlock_t* turnstileFsmTableGet(word_t *pMaxNum)
{
  *pMaxNum = TBL_SIZE(s_turnstileFsmBlks);
  return s_turnstileFsmBlks;
}





int turnstileFsmInit(zFsmPanel_t* panel)
{
  panel->fnInstGet  = turnstileInstanceGet;
  panel->fnTraceGet = turnstileFsmTraceGet;
  panel->fnStateName= turnstileStateName;
  panel->fnEventName= turnstileEventEnum2Name;

  panel->fnStateGet = turnstileStateGet;
  panel->fnStateSet = turnstileStateSet;
  panel->fnTableGet = turnstileFsmTableGet;

  panel->fnMsgPrint = turnstileMsgPrint;

  word_t maxNum = 0;
  zFsmBlock_t *pTable = panel->fnTableGet(&maxNum);
  if(!pTable || !maxNum)
  {
    zTraceFatal("invalid input fsm table, panel: %p.\n", panel);
    return -1;
  }

  //sort the table, so fsm could select an action quickly
  zFsmTableFormat(pTable, maxNum);

  return 0;
}

int turnstileInit()
{
  int i;

  int maxFsmTrace = 500;
  s_turnstileFsmServiceCore = zFsmServiceInit(maxFsmTrace);
  if(!s_turnstileFsmServiceCore)
  {
    zTraceFatal("Failed to create serviceCore, maxTrace=%d.\n", maxFsmTrace);
    return -1;
  }

  turnstileFsmInit(&s_turnstileFsmPanel);
  
  zMemset(&s_turnstileList, 0, sizeof(s_turnstileList));

  for(i=0; i<sizeof(s_turnstileList)/sizeof(s_turnstileList[0]); i++)
  {
    s_turnstileList[i].id = i+1;
    s_turnstileList[i].desc = "Turnstile";
    s_turnstileList[i].wState = eStateLocked;
  }

  return 0;
}

int GoodCoin(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceInfo("%s %d:: Welcome.\n", pInst->desc, pInst->id);
  return 0;
}
int GoodPush(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceInfo("%s %d:: See you next time.\n", pInst->desc, pInst->id);
  return 0;
}
int BadCoin(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceWarn("%s %d:: Thanks for your lenience...\n", pInst->desc, pInst->id);
  return 0;
}
int BadPush(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceWarn("%s %d:: You forgot to pay...\n", pInst->desc, pInst->id);
  return 0;
}
int OOS(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceWarn("%s %d:: Out of service...\n", pInst->desc, pInst->id);
  return 0;
}
int Attack(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceWarn("%s %d:: Attack warning...\n", pInst->desc, pInst->id);
  return eEventAudit;
}
int Audit(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  int auditFailRand = 0;
  
  zTraceInfo("%s %d:: Audit...\n", pInst->desc, pInst->id);

  auditFailRand = zTime();
  auditFailRand &= 0x1;

  zSleepUSec(700);
 
  return (auditFailRand ? eEventOos : 0);
}
int Repair(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceWarn("%s %d:: Under repairing...\n", pInst->desc, pInst->id);
  return 0;
}

int Fixed(word_t wEvent, turnstile_t *pInst, zMsgHdr_t *pMsg, zFsmContext_t* pContext)
{
  zTraceWarn("%s %d:: Fixed issue...\n", pInst->desc, pInst->id);
  return 0;
}

int turnstileScheduler(zMsgHdr_t *pMsg)
{
  zFsmCacheQueue_t cacheQueue;

  zMemset(&cacheQueue, 0, sizeof(cacheQueue));

  int nInst = pMsg->dstInst;
  word_t wEvent = turnstileEventGet(pMsg);

  int maxDepth = 20;
  while(maxDepth-- > 0)
  {
    int iRet = zFsmProcessMsg(s_turnstileFsmServiceCore, &s_turnstileFsmPanel, wEvent, nInst, pMsg, &cacheQueue.context);  
    if(iRet < 0)
    {
      zTraceError("Failed to handle event: %d %s   msg: %d %s\n", wEvent, turnstileEventEnum2Name(wEvent),
                                                        pMsg->msgId, turnstileMsgEnum2Name(pMsg->msgId));
      return -1;
    }
    else
    if(iRet == wEvent)
    {
      zTraceError("Can't handle event: %d %s     msg: %d %s\n", wEvent, turnstileEventEnum2Name(wEvent),
                                                        pMsg->msgId, turnstileMsgEnum2Name(pMsg->msgId));
      return -2;
    }
    else
    if(iRet > 0)
    {
      wEvent = iRet;
    }
    else
    if(iRet == 0)
    {
      int iRet2 = zFsmCacheDequeue(&cacheQueue.context, &wEvent, &pMsg, (dword_t *)&nInst);
      if(iRet2 == 0)
      {
        break; //no more event left, then quit the message process
      }
      else if(iRet2 < 0)
      {
        zTraceError("Failed to dequeue cache after event: %d %s, msg: %d, %s\n", wEvent, turnstileEventEnum2Name(wEvent),
                                                        pMsg->msgId, turnstileMsgEnum2Name(pMsg->msgId));
        return -3;
      }
    }
  }
  
  return 0;
}

int turnstileMsgSim(int msgId, int nInst)
{
  zMsgHdr_t ts_msg;

  zMemset(&ts_msg, 0, sizeof(ts_msg));

  //ts_msg.srcAddr = 0x12345678;
  //ts_msg.dstAddr = 0x87654321;
  ts_msg.srcInst = 0x9;
  ts_msg.dstInst = nInst;
  ts_msg.msgId = msgId;
  ts_msg.msgLen = 0;
  
  return turnstileScheduler(&ts_msg);
}


int turnstileFsmTraceShow(int traceId, int bEosFlag)
{
  zFsmTraceShow(s_turnstileFsmServiceCore, traceId, bEosFlag);
  return 0;
}

int turnstileInstanceShow(int nInst)
{
  if(nInst > 0 && nInst < TBL_SIZE(s_turnstileList))
  {
    zTraceP("turnstile instance:\n");
    zTraceP("[%d] %s\n", nInst, s_turnstileFsmPanel.fnStateName(s_turnstileList[nInst].wState));
    return 0;
  }
  
  int i;

  zTraceP("turnstile list:\n");
  for(i=1; i<TBL_SIZE(s_turnstileList); i++)
  {    
    zTraceP("[%d] %s\n", i, s_turnstileFsmPanel.fnStateName(s_turnstileList[i].wState));
  }
  
  return 0;
}

int turnstileShow()
{  
  zTraceP("turnstile FSM tablen\n");  
  zFsmTableShow(&s_turnstileFsmPanel); 
  zTraceP("turnstile FSM panel:\n");
  zFsmPanelShow(&s_turnstileFsmPanel);

  turnstileInstanceShow(0);

  zTraceP("turnstile FSM trace:\n");
  turnstileFsmTraceShow(0, 0);

  int i;
  zTraceP("turnstile FSM MSG:\n");
  for(i=eMsgCoin; i<eMsgRepair+1; i++) zTraceP("ID %d %s\n", i, turnstileMsgEnum2Name(i));
  zTraceP("turnstile FSM STATE:\n");
  for(i=eStateLocked; i<eStateUnknown+1; i++) zTraceP("ID %d %s\n", i, turnstileStateName(i));

  return 0;
}

int tsSend(char *msgname, int nInst)
{
  if(!s_turnstileFsmServiceCore) turnstileInit();

  if(!msgname || nInst <= 0 || nInst >= TBL_SIZE(s_turnstileList))
  {
    zTraceP("bad input name or instance (1~4).\n");
    zTraceP("tsTest(char *name, int nInst)");
    return -1;
  }
  
  int msgId = 0;  
  if(strcasestr("eMsgCoin", msgname)) msgId = eMsgCoin;
  else if(strcasestr("eMsgPush", msgname)) msgId = eMsgPush;
  else if(strcasestr("eMsgAttack", msgname)) msgId = eMsgAttack;
  else if(strcasestr("eMsgAudit", msgname)) msgId = eMsgAudit;  
  else if(strcasestr("eMsgFixed", msgname)) msgId = eMsgFixed;
  else if(strcasestr("eMsgRepair", msgname)) msgId = eMsgRepair;  
  else msgId = 6;

  return turnstileMsgSim(msgId, nInst);
}

int tsFsmShow(int nInst, int bEosFlag)
{
  if(nInst <= 0 || nInst >= TBL_SIZE(s_turnstileList))
  {
    return turnstileFsmTraceShow(0, 0);
  }
  return turnstileFsmTraceShow(s_turnstileList[nInst].traceId, bEosFlag);
}
int tsShow()
{
  if(!s_turnstileFsmServiceCore) turnstileInit();
  return turnstileShow();
}

