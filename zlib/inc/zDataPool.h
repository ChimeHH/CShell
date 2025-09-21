/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_DATAPOOL_H__
#define __Z_DATAPOOL_H__

#include "zType_Def.h"
#include "zDataPoolApi.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Z_DATAPOOL_TYPE
{
  char   name[32];
  char   function[32];
  int    line;
  int    shmKey;
  
  dword_t maxCount;
  dword_t dataSize;
  dword_t freeCount;
  dword_t mapNext;
  dword_t mapSize;

  dword_t magicValue;
  int     tCreate;

  FunDataPrintEach_t fnPrintEach;

  int     tLastAudit;
  
  sem_t sem;
  void*  pBlocks;
  byte_t bitMap[1];
} PACKED zDataPool_t;

#define DATAPOOL_SEM_INIT(pPool) sem_init(&(((zDataPool_t *)pPool)->sem), 0, 1);
#define DATAPOOL_SEM_WAIT(pPool) sem_wait(&(((zDataPool_t *)pPool)->sem))
#define DATAPOOL_SEM_POST(pPool) sem_post(&(((zDataPool_t *)pPool)->sem))
#define DATAPOOL_SEM_DEL(pPool) do{}while(0)


#ifdef __cplusplus
}
#endif

#endif /*__Z_DATAPOOL_H__*/






