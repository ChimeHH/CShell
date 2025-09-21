/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"
#include "zSalOS.h"

#include "cshell.h"
#include "zDataPool.h"
#include "zDataPoolApi.h"

#include "zTraceApi.h"
#include "zFootprintApi.h"

#include "cshell.h"
#include "zGlobal.h"

void* DataPoolCreateEx(const char* name, FunDataPrintEach_t fnPrintEach, int maxCount, int dataSize, int bReentrant, int bHA, const char* function, int line)
{
  int size;  
  zDataPool_t *pPool = 0;

  int tNow;

  tNow = (int)time(0);

  dataSize = (dataSize+7)>>3<<3;
  
  maxCount = (maxCount+7)>>3<<3; //to align bitMap

  int mapSize = maxCount>>3;
  mapSize  = (mapSize+7)>>3<<3;
  
  int blksSize = maxCount*dataSize;
  
  size = sizeof(zDataPool_t) + mapSize + blksSize;
  
  int key = zIpcStr2Key(name);

  zTraceDebug("name: %s, maxCount: %d, dataSize: %d, HA: %d,  @%s:%d\n", 
              name, maxCount, dataSize, bHA, function, line);
  
  //if bHA is true, then we use shm instead of heap memory
  if(!bHA || !key) goto init_mem;
  
  int shmId = 0;
  int bExist = 0;
  pPool = zShmGetEx(key, size, &shmId, &bExist, function, line);
  if(!pPool)
  {
    zTraceAlarm("mapping %s failed @%s:%d.\n", name, function, line);
    EOS_SET(shmget-ret-null-pool);
    goto init_mem;
  }

  if(!bExist)
  {
    EOS_SET(shmget-ret-new-pool);
    memset(pPool, 0, size);
    goto init_mem;
  }
  
  int size2 = zShmSize(shmId);
  if((size != size2)
     || (pPool->magicValue != MAGIC_VALUE)
     || (pPool->dataSize != dataSize) || (pPool->maxCount != maxCount))
  {
    zTraceAlarm("name: %s, total: %d (%d) magic: %08x (%08x) maxCount: %d (%d), dataSize: %d (%d), HA: %d,  @%s:%d\n",
                name, 
                size, size2, 
                pPool->magicValue, MAGIC_VALUE,
                pPool->maxCount, maxCount, 
                pPool->dataSize, dataSize, 
                bHA, function, line);

    if(size != size2) zEosPeg(name, size, size2, __FUNCTION__, __LINE__);
    else if(pPool->magicValue != MAGIC_VALUE) zEosPeg(name, pPool->magicValue, MAGIC_VALUE, __FUNCTION__, __LINE__);
    else if(pPool->dataSize != dataSize) zEosPeg(name, pPool->dataSize, dataSize, __FUNCTION__, __LINE__);
    else if(pPool->maxCount != maxCount) zEosPeg(name, pPool->maxCount, maxCount, __FUNCTION__, __LINE__);
    else zEosPeg(name, 0, 0, __FUNCTION__, __LINE__);
    
    zShmDistory(shmId);
        
    pPool = zShmGet(key, size, &shmId, &bExist);
    if(pPool)
    {
      EOS_SET(shmget-ret-resize-pool);
      memset(pPool, 0, size);
    }
    else
    {
      EOS_SET(shmget-realloc-failed);
    }
  }
  else
  {
    zTraceAlarm("mapping %s success @%s:%d.\n", name, function, line);
  }

  init_mem:

  if(!pPool) //bHA is false; or shmget failed, then use regular heap memory
  {   
    pPool = (zDataPool_t *)malloc(size);
    if(!pPool)
    {
      EOS_SET(data-pool-mem-failed);
      
      zTraceFatal("Memory downflow size=%d.\n", size);
      return 0;
    }

    EOS_SET(malloc-malloc-pool);
    memset(pPool, 0, size);
  }
  
  if(name) strncpy(pPool->name, name, TBL_SIZE(pPool->name)-1);
  strncpy(pPool->function, function, TBL_SIZE(pPool->function)-1);
  pPool->line = line;
  pPool->shmKey = key;
  
  pPool->maxCount  = maxCount;
  pPool->dataSize  = dataSize;
  pPool->freeCount = maxCount;
  pPool->mapSize   = mapSize;
  pPool->magicValue = MAGIC_VALUE;
  pPool->tCreate    = tNow;
  pPool->tLastAudit = tNow;

  pPool->fnPrintEach = fnPrintEach;

  pPool->bitMap[0] |= 0x01; //reserve 0, never use it.
  pPool->mapNext = 0;

  //pPool->pBlocks = (void*)pPool + size - maxCount*dataSize;
  pPool->pBlocks = (void*)pPool->bitMap + mapSize;

  DATAPOOL_SEM_INIT(pPool);

  zServiceCoreAdd(pPool, eServiceDATAPOOL, name, __FUNCTION__, __LINE__);

  return pPool;
}

inline int _DataPoolDestory(void* pool)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pPool)
  {
    return -1;
  }
  DATAPOOL_SEM_DEL(pPool);
  
  free(pPool);
  
  return 0;
}

int DataBlockAudit(void* pool, int bSet)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool)
  {
    zTraceWarn("Bad input null pool.\n");
    return -1;
  }
  if(!bSet)
  {
    return (int)time(0) - pPool->tLastAudit;
  }
  
  pPool->tLastAudit = (int)time(0);
  
  return 0;
}

int DataBlockMaxId(void* pool)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool)
  {
    zTraceWarn("Bad input null pool.\n");
    return -1;
  }

  pPool->tLastAudit = (int)time(0);

  return pPool->maxCount - 1;
}

int   DataBlockFreeCount(void* pool)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool)
  {
    zTraceWarn("Bad input null pool.\n");
    return -1;
  }

  return pPool->freeCount;
}

inline int DataBlockAlloc(void* pool, void *ppInstance)
{
  int i;

  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool)
  {
    zTraceWarn("Bad input null pool.\n");
    return -1;
  }

  DATAPOOL_SEM_WAIT(pPool);
  
  for(i=pPool->mapNext; i<pPool->mapSize; i++)
  {
    int j;

    if(pPool->bitMap[i] == 0xFF) continue;

    for(j=0; j<8; j++)
    {
      if( !(pPool->bitMap[i] & (1<<j)) )
      {
        pPool->bitMap[i] |= 1<<j;
        pPool->freeCount -= 1;

        pPool->mapNext = i;

        if(ppInstance) *(void**)ppInstance = pPool->pBlocks + pPool->dataSize*((i<<3)+j);

        DATAPOOL_SEM_POST(pPool);
        
        return (i<<3)+j;
      }
    }
  }
  //research from the head
  for(i=0; i<pPool->mapNext; i++)
  {
    int j;

    if(pPool->bitMap[i] == 0xFF) continue;

    for(j=0; j<8; j++)
    {
      if( !(pPool->bitMap[i] & (1<<j)) )
      {
        pPool->bitMap[i] |= 1<<j;
        pPool->freeCount -= 1;

        pPool->mapNext = i;
        
        if(ppInstance) *(void**)ppInstance = pPool->pBlocks + pPool->dataSize*((i<<3)+j);
        
        DATAPOOL_SEM_POST(pPool);
        
        return (i<<3)+j;
      }
    }
  }

  DATAPOOL_SEM_POST(pPool);
  
  zTraceWarn("Data pool downflow.\n");
  return -1;
}

int DataBlockFree(void* pool, int nIndex)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool || nIndex>=pPool->maxCount)
  {
    zTraceWarn("Bad input pool %p or index %d.\n", pPool, nIndex);
    return -1;
  }

  if( pPool->bitMap[nIndex>>3] & (1 << (nIndex & 0x7)) )
  {
    pPool->bitMap[nIndex>>3] ^= (1 << (nIndex & 0x7));
    pPool->freeCount += 1;

    return 0;
  }

  return -2;
}

int DataBlockSetBusy(void* pool, int nIndex)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool || nIndex>=pPool->maxCount)
  {
    zTraceWarn("Bad input pool %p or index %d.\n", pPool, nIndex);
    return -1;
  }
  
  if(pPool->bitMap[nIndex>>3] & (1 << (nIndex & 0x7))) 
    return 0;
  
  pPool->bitMap[nIndex>>3] |= (1 << (nIndex & 0x7));
  pPool->freeCount -= 1;

  return 1;
}

void *DataBlockMap(void* pool, int nIndex)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool || nIndex<=0 || nIndex>=pPool->maxCount)
  {
    zTraceWarn("Bad input pool %p or index %d.\n", pPool, nIndex);
    return 0;
  }

  if( pPool->bitMap[nIndex>>3] & (1 << (nIndex & 0x7)) )
  {
    return pPool->pBlocks + pPool->dataSize*nIndex;
  }

  return 0;
}

void *DataBlockMapEx(void* pool, int nIndex, int *bUsedFlag)
{
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool || nIndex<0 || nIndex>=pPool->maxCount)
  {
    zTraceWarn("Bad input pool %p or index %d.\n", pPool, nIndex);
    return 0;
  }

  if( pPool->bitMap[nIndex>>3] & (1 << (nIndex & 0x7)) )
  {
    if(bUsedFlag) *bUsedFlag = 1;
  }
  else
  {
    if(bUsedFlag) *bUsedFlag = 0;
  }

  return pPool->pBlocks + pPool->dataSize*nIndex;;
}

static int DataPoolEachDefault(int nIndex, int bUsedFlag, void* pInstance, int dataSize, int userData)
{
  zTraceP("%4d  %d  %4d : ", nIndex, bUsedFlag, dataSize);
  zTraceHexP(pInstance, dataSize);
  zTraceP("\n");

  return 0;
}

int DataPoolWalk(void* pool, zFunc_t fnEach, int userData)
{
  int i;

  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool)
  {
    zTraceWarn("Bad input null pool.\n");
    return -1;
  }

  if(!fnEach) fnEach = (zFunc_t)DataPoolEachDefault;

  for(i=0; i<pPool->maxCount; i++)
  {
    void *pInstance = pPool->pBlocks + i*pPool->dataSize;
    
    int bUsedFlag = 0;    
    if( pPool->bitMap[i>>3] & (1<<(i&0x7)) ) bUsedFlag = 1;

    fnEach(i, bUsedFlag, pInstance, pPool->dataSize, userData);
  }

  return 0;
}




int DataPoolShow(void* pool, int bForceFlag)
{
  int i;
  zDataPool_t *pPool = (zDataPool_t *)pool;
  if(!pool)
  {
    zTraceWarn("Bad input null pool.\n");
    return -1;
  }
  
  zTraceP("name  : %s %d\n", pPool->name, pPool->shmKey);
  zTraceP("function  : %s %d\n", pPool->function, pPool->line);
  
  zTraceP("maxCount  : %d\n", pPool->maxCount);
  zTraceP("dataSize  : %d\n", pPool->dataSize);
  zTraceP("freeCount : %d\n", pPool->freeCount);
  zTraceP("mapNext   : %d\n", pPool->mapNext);
  zTraceP("mapSize   : %d\n", pPool->mapSize);
  
  zTraceP("magicValue: %08x\n", pPool->magicValue);
  zTraceP("tCreate   : %d\n", pPool->tCreate);
  zTraceP("tAudit: %d, (%d seconds before)\n", pPool->tLastAudit, (int)time(0)-pPool->tLastAudit); 
  zTraceP("fnPrintEach: %s\n", pPool->fnPrintEach ? symFuncNameGet(pPool->fnPrintEach) : "Not defined");
  zTraceP("pBitMap: %p,  pBlocks: %p\n", pPool->bitMap, pPool->pBlocks);
  zTraceP("BIT MAP:\n");
  zTraceHexP(pPool->bitMap, pPool->mapSize);
  zTraceP("\n");

  for(i=0; i<pPool->maxCount; i++)
  { 
    int bUsed = pPool->bitMap[i>>3] & (1<<(i&0x7));
    void *pInst = pPool->pBlocks + i*pPool->dataSize;
    
    if(pPool->fnPrintEach)
      pPool->fnPrintEach(i, bUsed, pInst, pPool->dataSize, bForceFlag);
    else if(bUsed || bForceFlag)
    {      
      zTraceP("%4d  %4d : ", i, pPool->dataSize);
      zTraceHexP(pInst, pPool->dataSize);
      zTraceP("\n");
    }
  }

  return 0;
}



#if 0 //DATAPOOL_SELFTEST
int testDataPool()
{
  int nIndex;
  void *pool = DataPoolCreate("TEST DATA", 0, 20, 5, 0);

  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));
  nIndex = DataBlockAlloc(pool, 0); zTraceP("alloc %d %p\n", nIndex, DataBlockMap(pool, nIndex));

  DataPoolShow(pool);
  DataPoolWalk(pool, 0, 0);

  DataBlockFree(pool, 0);
  DataBlockFree(pool, 1);
  DataBlockFree(pool, 2);
  DataBlockFree(pool, 3);  
  DataBlockFree(pool, 5);
  DataBlockFree(pool, 6);  
  DataBlockFree(pool, 7);
  DataBlockFree(pool, 8);
  DataBlockFree(pool, 3);
  DataPoolShow(pool);

  return 0;
}

#endif /*DATAPOOL_SELFTEST*/

