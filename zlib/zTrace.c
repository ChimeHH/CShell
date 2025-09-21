/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
       
#include "zType_Def.h"
#include "zSalOS.h"

#include "zTrace.h"
#include "zTraceApi.h"
#include "cshell.h"

//#define g_zTraceDefaultLevel  (TraceFatal | TraceAlarm | TraceError | TraceDebug | TraceInfo | TraceWarn)
#define g_zTraceDefaultLevel  (TraceFatal | TraceAlarm | TraceError)

int g_zTraceCommonLevel = g_zTraceDefaultLevel;


static ZTracCtrlT szTraceCtrl;


pthread_spinlock_t g_zTraceLock = 0;

int zTraceBlockOn()
{
  pthread_spin_lock(&g_zTraceLock);
  return 0;
}

int zTraceBlockOff()
{
  pthread_spin_unlock(&g_zTraceLock);
  return 0;
}

void* g_zShellTaskId = 0;

int zTraceServiceInit()
{ 
  memset(&szTraceCtrl, 0, sizeof(szTraceCtrl));

  g_zTraceCommonLevel = g_zTraceDefaultLevel;

  pthread_spin_unlock(&g_zTraceLock);


  return 0;
}

int zTraceLevelReset()
{
  zTraceServiceInit();

  return 0;
}


int zTraceTaskLevel()
{
  void* tid;
  
  if(!szTraceCtrl.enable)
  {
    return g_zTraceCommonLevel;
  }
  
  tid = zTaskSelf();
  
  if(szTraceCtrl.count>0)
  {
    int i;
    for(i=0; i<szTraceCtrl.count; i++)
    {
      if(szTraceCtrl.nodal[i].tid != tid) continue;
      
      if(szTraceCtrl.nodal[i].bLogFlag) return TRACE_ALL_LEVEL;
      
      return szTraceCtrl.nodal[i].level;
    }
  }
  
  return g_zTraceCommonLevel;
}



int zTraceLevelSet(void* tid, int level)
{
  int i;
  
  if(!tid)
  {
    tid = zTaskSelf();
  }

  if(!tid) return -1;

  if(level < 0) level = g_zTraceCommonLevel;

  szTraceCtrl.enable = 1;

  for(i=0; i<szTraceCtrl.count; i++)
  {  
    if(szTraceCtrl.nodal[i].tid == tid)
    {
      szTraceCtrl.nodal[i].level = level;
      return 0;
    }
  }

  //not found, then add one
  if(szTraceCtrl.count >= TBL_SIZE(szTraceCtrl.nodal))
  {
    zTraceAlarm("trace nodal list overflow.\n");
    return -1;
  }

  memset(&szTraceCtrl.nodal[i], 0, sizeof(szTraceCtrl.nodal[i]));
  szTraceCtrl.nodal[i].tid = tid;
  szTraceCtrl.nodal[i].level = level;
  
  szTraceCtrl.count += 1;

  return 0;
}

int zTraceFlagSet(void* tid, int bLogFlag)
{
  int i;

  return 0;
  
  if(!tid)
  {
    tid = zTaskSelf();
  }

  if(!tid) return -1;

  if(bLogFlag) szTraceCtrl.enable = 1;

  for(i=0; i<szTraceCtrl.count; i++)
  {  
    if(szTraceCtrl.nodal[i].tid == tid)
    {
      szTraceCtrl.nodal[i].bLogFlag = bLogFlag;
      return 0;
    }
  }

  //not found, then add one
  if(szTraceCtrl.count >= TBL_SIZE(szTraceCtrl.nodal))
  {
    zTraceWarn("trace nodal list overflow.\n");
    return -1;
  }

  memset(&szTraceCtrl.nodal[i], 0, sizeof(szTraceCtrl.nodal[i]));
  szTraceCtrl.nodal[i].tid = tid;
  szTraceCtrl.nodal[i].bLogFlag = bLogFlag;
  
  szTraceCtrl.count += 1;

  return 0;
}


int zTraceLevelSetAll(int level)
{  
  szTraceCtrl.enable = 0;
  szTraceCtrl.count = 0;
  
  g_zTraceCommonLevel = level;

  return g_zTraceCommonLevel;
}
 
int zTraceLevelShow()
{
  int i;

  zTraceP("common level: %08x  default: %08x   Enable Special Flag: %d\n",
                szTraceCtrl.enable,
                g_zTraceCommonLevel,
                g_zTraceDefaultLevel);
  
  for(i=0; i<szTraceCtrl.count; i++)  
  {
    zTraceP("THREAD: %p %s  LEVEL: %08x bLogFlag:%d\n", 
                szTraceCtrl.nodal[i].tid, zTaskId2Name(szTraceCtrl.nodal[i].tid),
                szTraceCtrl.nodal[i].level,
                szTraceCtrl.nodal[i].bLogFlag);    
  }

  return 0;
}


int testTrace(int a, char b)
{
  int hex[] = {1, 2, 3, 4};
  
  
  zTraceFatal("hahhaha %d\n", 2);
  zTraceAlarm("hahhaha %d\n", 2);
  zTraceError("hahhaha %d\n", 2);
  zTraceWarn ("hahhaha %d\n", 2);
  zTraceInfo ("hahhaha %d\n", 2);
  zTraceDebug("hahhaha %d\n", 2);
  
  zTraceFatalQ("hahhaha %d\n", 2);
  zTraceAlarmQ("hahhaha %d\n", 2);
  zTraceErrorQ("hahhaha %d\n", 2);
  zTraceWarnQ ("hahhaha %d\n", 2);
  zTraceInfoQ ("hahhaha %d\n", 2);
  zTraceDebugQ("hahhaha %d\n", 2);
  zTraceP     ("hahhaha %d\n", 2);
  
  zTraceHexFatal(hex, 16);
  zTraceHexAlarm(hex, 16);
  zTraceHexError(hex, 16);
  zTraceHexWarn (hex, 16);
  zTraceHexInfo (hex, 16);
  zTraceHexDebug(hex, 16);
  
  zTraceHexFatalQ(hex, 16);
  zTraceHexAlarmQ(hex, 16);
  zTraceHexErrorQ(hex, 16);
  zTraceHexWarnQ (hex, 16);
  zTraceHexInfoQ (hex, 16);
  zTraceHexDebugQ(hex, 16);

  zTraceHexP     (hex, 16);


  return 0;
}


int z_IamShell()
{
  g_zShellTaskId = zTaskSelf();
  return 0;
}

int z_InShell()
{
  if(g_zShellTaskId == zTaskSelf())
  {
    return 1;
  }
  return 0;
}

int z_InKernal()
{
  return 0;
}

int z_InInterrupt()
{
  return 0;
}

int tcset(const char* taskName, const char* funName, int level)
{
  void* entry = 0;
  
  if(funName) entry = symFindByName(funName, 0, 0, 0);
  void* tid = zTaskIdEx(taskName, entry);
  
  if(!tid)
  {
    zTraceP("can't find the task.\n");
    return -1;
  }

  return zTraceLevelSet(tid, level);
}

int tcshow()
{
  return zTraceLevelShow();
}

