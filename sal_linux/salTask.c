/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <string.h>
char *strcasestr(const char *haystack, const char *needle);
       
#include "zType_Def.h"
#include "zSalOS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <pthread.h>
#include "zTraceApi.h"
#include "cshell.h"


//////////////////////////////////////////////////////////////////////////////
//pthread don't provide a list of threads in process, unless we have a special
//module (using task_struc, next_thread), or read filesystem.
//salTask creates a simple static list to make thing simple. However, if overflow
//happens, then just discard new threads.
struct SalTask_t{
  void*   tid; 
  const char* name;
  void* entry;
  void* arg;  
};

static struct SalTask_t sTaskList[128] = { {0, }, };
static int task_add(void* tid, const char* name, void* entry, void* arg)
{
  int i;
  for(i=0; i<TBL_SIZE(sTaskList); i++)
  {
    if(sTaskList[i].entry) continue;

    sTaskList[i].tid = tid;
    sTaskList[i].name = name;
    sTaskList[i].entry = entry;
    sTaskList[i].arg = arg;
    
    return i+1;
  }

  return 0;
}

const char* zTaskId2Name(void* tid)
{
  int i;

  if(!tid) return 0;
  
  for(i=0; i<TBL_SIZE(sTaskList); i++)
  {
    if(sTaskList[i].tid == tid) return sTaskList[i].name;
  }

  return 0;
}

void* zTaskIdEx(const char* name, void* entry)
{
  int i;
  for(i=0; i<TBL_SIZE(sTaskList); i++)
  {
    if(!sTaskList[i].entry) continue;
    
    if(name && sTaskList[i].name && strcasestr(sTaskList[i].name, name)) return sTaskList[i].tid;
    if(sTaskList[i].entry == entry) return sTaskList[i].tid;
  }

  return 0;
}

int zTaskList()
{
  int i;
  for(i=0; i<TBL_SIZE(sTaskList); i++)
  {
    if(!sTaskList[i].entry) continue;

    zTraceP("[%d] thread: %p,  name: %s, entry: %s, arg: %p\n", i+1,
               sTaskList[i].tid, sTaskList[i].name, 
               symFuncNameGet(sTaskList[i].entry), sTaskList[i].arg);    
  }

  return 0;
}

void* zTaskCreateEx(const char* taskName, void *(*fnEntry) (void *), void* arg)
{
  int iRet;  
  pthread_t thread;
  
  iRet = pthread_create(&thread, 0, fnEntry, arg);
  if(iRet != 0)
  {
    zTraceP("pthread_create failed: %d.\n", iRet);
    return 0;
  }

  task_add((void*)thread, taskName, fnEntry, arg);

  return (void*)thread;  
}

void* zTaskSelf()
{
  return (void*)pthread_self();
}

int zTaskEqual(void* tid1, void* tid2)
{
  return pthread_equal((pthread_t)tid1, (pthread_t)tid2);
}

int zTaskStop(void* tid)
{
  return pthread_cancel((pthread_t)tid);
}

void *testTaskEntry(void *arg)
{
  int a =0;

  if(arg) a = *(int*)arg;

  while(1)
  {
    zTraceP("Task [%p, %ld]  arg: %d\n", zTaskSelf(), (long)zTaskSelf(), a);

    sleep(5);
  }

  return 0;
}


