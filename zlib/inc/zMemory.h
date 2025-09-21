/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_MEMORY_H__
#define __Z_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MEM_RATIO  6
#define MEM_BASE   64

#define MEM_VDBG_SIZE  sizeof(zMemVdbg_t)

#define MEM_RSIZE(as)  (((as) + MEM_VDBG_SIZE + 63)>>6)
#define MEM_ASIZE(rs)  (((rs)<<6) - MEM_VDBG_SIZE)

#define MEM_ID2MAGIC(id)  (((id)>>24)&0x7F)
#define MEM_ID2BLK(id)    ((id)&0xFFFFFF)

#define MEM_ADDR(head, nblk)  ((void*)(head) + ((nblk)<<6) + MEM_VDBG_SIZE)
#define MEM_VADDR(addr)       ((zMemVdbg_t*)((void*)(addr) - MEM_VDBG_SIZE))

typedef struct Z_MEMORY_MBLK_TYPE
{
  dword_t used:1;
  dword_t magic:7;
  dword_t count:24; //actual = size<<6 = size*64, and total upto 30bits=1Gbytes
} PACKED zMemMblk_t;

#define MEM_VERBOSE_DEBUG 1
typedef struct Z_MEMORY_DBLK_TYPE
{
#ifdef MEM_VERBOSE_DEBUG
  const char* fnAlloc;
  int   lineAlloc;
  int   timeAlloc;
#endif
} zMemVdbg_t;


typedef struct Z_MEMORY_POOL_TYPE
{
  sem_t       sem;
  dword_t     maxBlkCount;  
  dword_t     nextBlkId;
  dword_t     busyBlkCount;
    
  zMemMblk_t *mblkHead;
  void       *dblkHead;
} zMemPool_t;

#define MP_SEM_INIT(memPool) sem_init(&(((zMemPool_t *)memPool)->sem), 0, 1);
#define MP_SEM_WAIT(memPool) sem_wait(&(((zMemPool_t *)memPool)->sem))
#define MP_SEM_POST(memPool) sem_post(&(((zMemPool_t *)memPool)->sem))
#define MP_SEM_DEL(memPool)  do{}while(0)


#ifdef __cplusplus
}
#endif

#endif /*__Z_MEMORY_H__*/






