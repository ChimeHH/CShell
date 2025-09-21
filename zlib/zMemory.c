/*----------------------------------------------------------
File Name  : xxx.c
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"
#include "zSalOS.h"
#include "zTraceApi.h"

#include "zMemory.h"
#include "zMemoryApi.h"

#include "time.h"

void *zMemoryPoolCreate(const char* name, dword_t size, int bReentrant)
{
  zMemPool_t *memPool;
  int len;

  dword_t maxBlkCount = MIN(0xFFFFFF, MEM_RSIZE(size));

  len = sizeof(zMemPool_t) + (sizeof(zMemMblk_t) + MEM_BASE)*maxBlkCount;

  memPool = malloc(len);
  if(!memPool)
  {
    zTraceAlarm("memory downflow, total request %d, actuall %d\n", size, len);
    return 0;
  }

  memset(memPool, 0, len);
  memPool->maxBlkCount = maxBlkCount;
  memPool->mblkHead = (zMemMblk_t *)(memPool + 1);
  memPool->dblkHead = (void *)(memPool->mblkHead + maxBlkCount);

  memPool->mblkHead[0].count = maxBlkCount;

  if(bReentrant) MP_SEM_INIT(memPool);

  return memPool;
}

int zMemoryAlloc(void *pool, int size, void **ppMem, const char* function, int line)
{
  static dword_t magic = 0;

  zMemPool_t *memPool = (zMemPool_t *)pool;
  if(!memPool || size<=0)
  {
    zTraceWarn("Invalid null memory pool %p or request size %d\n", memPool, size);
    return -1;
  }

  int freeSize = MEM_ASIZE(memPool->maxBlkCount-memPool->busyBlkCount);
  if(size > freeSize)
  {
    zTraceWarn("memory poll downflow, request %d > free %d\n", size, freeSize);
    return -1;
  }

  dword_t rsize = MEM_RSIZE(size);

  MP_SEM_WAIT(memPool); //!!!!!!!!!!!!!Please ensure there's an unlock before each return

  zMemMblk_t *mblk, *mblk2;
  while(memPool->nextBlkId < memPool->maxBlkCount)
  {
    mblk = &memPool->mblkHead[memPool->nextBlkId];
    if(mblk->used)
    {
      memPool->nextBlkId += mblk->count;
      continue;
    }

    if(mblk->count >= rsize)
    {
      goto FOUND_FREE_BLK;
    }

    if(memPool->nextBlkId + mblk->count < memPool->maxBlkCount)
    {
      mblk2 = mblk + mblk->count;
      if(!mblk2->used)
      {
        mblk->count += mblk2->count;
        mblk2->used = 0;
        mblk2->count = 0;

        continue;
      }
    }

    memPool->nextBlkId += mblk->count;
  }

  //research from begin and merge fragements
  memPool->nextBlkId = 0;
  while(memPool->nextBlkId < memPool->maxBlkCount)
  {
    mblk = &memPool->mblkHead[memPool->nextBlkId];
    if(mblk->used)
    {
      memPool->nextBlkId += mblk->count;
      continue;
    }

    if(mblk->count >= rsize)
    {
      goto FOUND_FREE_BLK;
    }

    if(memPool->nextBlkId + mblk->count < memPool->maxBlkCount)
    {
      mblk2 = mblk + mblk->count;
      if(!mblk2->used)
      {
        mblk->count += mblk2->count;
        mblk2->used = 0;
        mblk2->count = 0;

        continue;
      }
    }

    memPool->nextBlkId += mblk->count;
  }

  MP_SEM_POST(memPool);  //!!!!!!!!!!!!!
  zTraceWarn("memory poll fragement fail, request %d vs. free %d\n", size, freeSize);
  return -1;

  FOUND_FREE_BLK:
  mblk->used = 1;
  magic = (magic+1) ? (magic+1) : 1;
  mblk->magic = 0x7F & (magic++);

  int ret = memPool->nextBlkId + (mblk->magic<<24);

  if(mblk->count > rsize)
  {
    mblk2 = mblk + rsize;

    mblk2->used = 0;
    mblk2->magic = 0;
    mblk2->count = mblk->count - rsize;
  }

  mblk->count = rsize;
  if(ppMem) *ppMem = MEM_ADDR(memPool->dblkHead, memPool->nextBlkId);

  memPool->busyBlkCount += mblk->count;

#ifdef MEM_VERBOSE_DEBUG
  zMemVdbg_t *vdbg = MEM_VADDR(MEM_ADDR(memPool->dblkHead, memPool->nextBlkId));
  vdbg->fnAlloc = function;
  vdbg->lineAlloc = line;
  vdbg->timeAlloc = (int)time(0);
#endif

  memPool->nextBlkId += rsize;

  MP_SEM_POST(memPool);
  return ret;
}

int zMemoryFreeEx(void *pool, int nMemId, int bAuth)
{
  zMemPool_t *memPool = (zMemPool_t *)pool;

  dword_t nBlk = MEM_ID2BLK(nMemId);
  if(!memPool)
  {
    zTraceWarn("Invalid null memory pool\n");
    return -1;
  }
  if(nBlk >= memPool->maxBlkCount)
  {
    zTraceWarn("Invalid memory id %d (x%08x), exceed max count %d\n", nMemId, nMemId, memPool->maxBlkCount);
    return -1;
  }

  zMemMblk_t *mblk = memPool->mblkHead + nBlk;
  if(!mblk->used || !mblk->count)
  {
    zTraceWarn("Invalid memory id %d (x%08x), idle block\n", nMemId, nMemId);
    return -1;
  }

  if(bAuth && mblk->magic != MEM_ID2MAGIC(nMemId))
  {
    zTraceWarn("Invalid memory id %d (x%08x), magic (x%02x) check failed\n", nMemId, nMemId, mblk->magic);
    return -1;
  }

  mblk->used = 0;
  mblk->magic = 0;

  MP_SEM_WAIT(memPool);
  memPool->busyBlkCount -= mblk->count;
  MP_SEM_POST(memPool);

  return 0;
}

int zMemoryFree(void *pool, int nMemId)
{
  return zMemoryFreeEx(pool, nMemId, 1);
}

int zMemoryDownsizing(void *pool, int nMemId, int newSize)
{
  zMemPool_t *memPool = (zMemPool_t *)pool;

  dword_t nBlk = MEM_ID2BLK(nMemId);
  if(!memPool || newSize<=0)
  {
    zTraceWarn("Invalid null memory pool %p, or bad newSize %d\n", pool, newSize);
    return -1;
  }
  if(nBlk >= memPool->maxBlkCount)
  {
    zTraceWarn("Invalid memory id %d (x%08x), exceed max count %d\n", nMemId, nMemId, memPool->maxBlkCount);
    return -1;
  }

  zMemMblk_t *mblk = memPool->mblkHead + nBlk;
  if(!mblk->used || !mblk->count)
  {
    zTraceWarn("Invalid memory id %d (x%08x), idle block\n", nMemId, nMemId);
    return -1;
  }

  if(mblk->magic != MEM_ID2MAGIC(nMemId))
  {
    zTraceWarn("Invalid memory id %d (x%08x), magic (x%02x) check failed\n", nMemId, nMemId, mblk->magic);
    return -1;
  }

  dword_t rsize = MEM_RSIZE(newSize);
  if(mblk->count <= rsize)
  {
    //no space to do downsizing
    return 0;
  }

  zMemMblk_t *mblk2 = mblk + (mblk->count - rsize);
  mblk2->count = (mblk->count - rsize);
  mblk2->used = 0;

  mblk->count = rsize;

  return 1;
}

void *zMemoryMap(void *pool, int nMemId, int *pLen)
{
  zMemPool_t *memPool = (zMemPool_t *)pool;

  dword_t nBlk = MEM_ID2BLK(nMemId);
  if(!memPool)
  {
    zTraceWarn("Invalid null memory pool\n");
    return 0;
  }
  if(nBlk >= memPool->maxBlkCount)
  {
    zTraceWarn("Invalid memory id %d, exceed max count %d\n", nMemId, memPool->maxBlkCount);
    return 0;
  }

  zMemMblk_t *mblk = memPool->mblkHead + nBlk;
  if(!mblk->used)
  {
    zTraceWarn("Invalid memory id %d, idle block\n", nMemId);
    return 0;
  }

  if(mblk->magic != MEM_ID2MAGIC(nMemId))
  {
    zTraceWarn("Invalid memory id %d, magic check failed\n", nMemId);
    return 0;
  }

  if(pLen) *pLen = MEM_ASIZE(mblk->count);

  return MEM_ADDR(memPool->dblkHead, nBlk);
}

int zMemoryPoolShow(void *pool, int bStat)
{
  zMemPool_t *memPool = (zMemPool_t *)pool;

  dword_t next = 0;
  if(!memPool)
  {
    zTraceP("Invalid null memory pool\n");
    return 0;
  }

  zTraceP("MEMORY POOL %p\n", pool);
  zTraceP("  .maxBlkCount : %d\n", memPool->maxBlkCount);
  zTraceP("  .nextBlkId   : %d\n", memPool->nextBlkId);
  zTraceP("  .busyBlkCount: %d\n", memPool->busyBlkCount);

  zTraceP("  .mblkHead    : %p\n", memPool->mblkHead);
  zTraceP("  .dblkHead    : %p\n", memPool->dblkHead);

  zMemMblk_t *mblk;
  void *addr;

  if(bStat)
  {
    zTraceP("INDEX       USED MAGIC   BLKS\n");
    for(next=0; next<memPool->maxBlkCount; next++)
    {
      mblk = &memPool->mblkHead[next];
      zTraceP("[%8d]    %1d  x%02x %8d\n", next, mblk->used, mblk->magic, mblk->count);
    }
    return 0;
  }

  zTraceP("INDEX       USED MAGIC   BLKS    ADDRESS                           FUNCTION   LINENO   TIME\n");
  while(next < memPool->maxBlkCount)
  {
    mblk = &memPool->mblkHead[next];

    addr = MEM_ADDR(memPool->dblkHead, next);

    if(!mblk->used)
    {
      zTraceP("[%8d]    %1d  x%02x %8d - %8p\n", next, mblk->used, mblk->magic, mblk->count, addr);
      next += mblk->count;
      continue;
    }

#ifdef MEM_VERBOSE_DEBUG
    zMemVdbg_t *vdbg = MEM_VADDR(addr);
    zTraceP("[%8d]    %1d  x%02x %8d - %8p %32s    %4d     %s\n", next, mblk->used, mblk->magic, mblk->count, addr, vdbg->fnAlloc, vdbg->lineAlloc, ctime((time_t *)&vdbg->timeAlloc));
#else
    zTraceP("[%8d]    %1d  x%02x %8d - %8p\n", next, mblk->used, mblk->magic, mblk->count, addr);
#endif

    next += mblk->count;
  }

  return 0;
}



