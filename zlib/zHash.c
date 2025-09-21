/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"
#include "zSalOS.h"

#include "cshell.h"

#include "zTraceApi.h"
#include "zList.h"
#include "zHash.h"
#include "zListApi.h"  

void *_HashCreate(const char *name, int nmemb, int width, zFunc_t fnCmpRtn, zFunc_t fnKeyRtn)
{
  HashTable_t *pTable;

  int size, log2Size;

  if(!name || !fnCmpRtn || !fnKeyRtn)
  {
    zTraceFatal("invalid null name, compare or key function.\n");

    return 0;
  }
  
  if(nmemb < 1024)
  {
    log2Size = 0xFFF;
  }
  else
  {
    log2Size = 0xFFFF;
  }

  size = sizeof(HashTable_t) + nmemb*NODE_SIZE(width) + log2Size*HASH_SLOT_SIZE + HASH_RESERVED_SIZE;  //table, nmemb*(data, next), out, magic
  pTable = (HashTable_t*)malloc(size);
  if(!pTable)
  {
    zTraceFatal("heap alloc failed\n");
    return 0;
  }

  memset(pTable, 0, size);  
  
  pTable->fnCmpRtn = fnCmpRtn;
  pTable->fnKeyRtn = fnKeyRtn;
  
  pTable->log2Size = log2Size;

  pTable->nodeList.maxCount = nmemb;
  pTable->nodeList.dataSize = width;
  pTable->nodeList.pNodePool = (void*)(pTable+1) + log2Size*HASH_SLOT_SIZE + HASH_RESERVED_SIZE;  //ignore the 4bytes leak tailing pTable

  zLineListInit(&pTable->nodeList);

  return pTable;
}

void* _zHashNodeFind(void *table, void *memb, int *pIndex)
{
  int key;
  int *pSlot;

  HashTable_t *pTable = (HashTable_t *)table;
  if(!table || !memb)
  {
    zTraceError("Invalid table %p, or memb %p\n", table, memb);
    return 0;
  }

  key = pTable->fnKeyRtn(memb) % pTable->log2Size;
  pSlot = &pTable->headSlot[key];

  while(*pSlot)
  {
    void *pData = LIST_NODE_DATA(&pTable->nodeList, *pSlot);
    int ret = pTable->fnCmpRtn(pData, memb);
    if(ret == 0)
    {
      if(pIndex) *pIndex = *pSlot;
      return pData;
    }

    pSlot = &LIST_NODE_NEXT(&pTable->nodeList, *pSlot);
  }

  return 0;
}

int _zHashNodeAdd(void * table, void * memb, zFunc_t fnUpdate, int dupCheck)
{
  int iRet;
  int nIndex;

  HashTable_t *pTable = (HashTable_t *)table;

  void *pData = 0;

  if(!table || !memb)
  {
    zTraceError("Invalid table %p, or memb %p\n", table, memb);
    return -1;
  }

  if(dupCheck)
  {
    pData = _zHashNodeFind(table, memb, &nIndex);
  }

  if(!pData)
  {  
    int key = pTable->fnKeyRtn(memb) % pTable->log2Size;

    iRet = zLineListAlloc(&pTable->nodeList, 0);
    if(iRet <= 0)
    {
      zTraceWarn("List %p downflow\n", &pTable->nodeList);
      return -2;
    }

    nIndex = iRet;

    pData = LIST_NODE_DATA(&pTable->nodeList, nIndex);

    zLineListLink(&pTable->nodeList, nIndex, &pTable->headSlot[key]);
  }

  if(fnUpdate)
  {
    fnUpdate( pData, memb );
  }
  else
  {
    memcpy( pData, memb, pTable->nodeList.dataSize );
  }

  return 0;
}

int _zHashNodeDel(void *table, void *memb)
{
  int key;
  int *pSlot;

  HashTable_t *pTable = (HashTable_t *)table;
  if(!table || !memb)
  {
    zTraceError("Invalid table %p, or memb %p\n", table, memb);
    return -1;
  }

  key = pTable->fnKeyRtn(memb) % pTable->log2Size;
  pSlot = &pTable->headSlot[key];

  while(*pSlot)
  {
    void *pData = LIST_NODE_DATA(&pTable->nodeList, *pSlot);
    int ret = pTable->fnCmpRtn(pData, memb);
    if(ret == 0)
    {
      int nIndex = *pSlot;
      zLineListUnlink(&pTable->nodeList, pSlot);
      zLineListFree(&pTable->nodeList, nIndex);
      
      return 1;
    }

    pSlot = &LIST_NODE_NEXT(&pTable->nodeList, *pSlot);
  }

  return 0;
}

int _zHashDestroy(void *table)
{
  if(table) free(table);

  return 0;
}




int _zHashNodeEach(void *table, zFunc_t fnEach)
{
  int key;
  int *pSlot;

  HashTable_t *pTable = (HashTable_t *)table;
  if(!table || !fnEach)
  {
    zTraceError("Invalid table %p, or fnEach %p\n", table, fnEach);
    return -1;
  }

  for(key=0; key<pTable->log2Size; key++)
  {
    pSlot = &pTable->headSlot[key];
    zTraceP("SLOT %d:\n", key);
    while(*pSlot)
    {
      void *pData = LIST_NODE_DATA(&pTable->nodeList, *pSlot);

      fnEach(pData);

      pSlot = &LIST_NODE_NEXT(&pTable->nodeList, *pSlot);
    }
    zTraceP("\n");
  }

  return 0;
}

int _zHashRebuild(void *table)
{
  int i, key;
  HashTable_t *pTable = (HashTable_t *)table;
  if(!table)
  {
    zTraceError("Invalid table %p\n", table);
    return -1;
  }

  zLineListRebuild(&pTable->nodeList);

  for(key=0; key<pTable->log2Size; key++)
  {
    pTable->headSlot[key] = 0;
  }

  pTable->nodeList.freeCount = 0;
  for(i=0; i<pTable->nodeList.maxCount; i++)
  {
    ListNode_t *pNode = LIST_NODE_PTR(&pTable->nodeList, i);

    if(!NODE_USED(pNode))
    {
      pTable->nodeList.freeCount += 1;
      continue;
    }

    key = pTable->fnKeyRtn(NODE_DATA(pNode)) % pTable->log2Size;
    zLineListLink(&pTable->nodeList, i, &pTable->headSlot[key]);
  }

  return 0;
}

void *zHashCreate(const char *name, int nmemb, int width, zFunc_t fnCmpRtn, zFunc_t fnKeyRtn)
{
  void* table = _HashCreate(name, nmemb, width, fnCmpRtn, fnKeyRtn);  
  return table;
}
void *zHashCreate_r(const char *name, int nmemb, int width, zFunc_t fnCmpRtn, zFunc_t fnKeyRtn)
{
  void* table = _HashCreate(name, nmemb, width, fnCmpRtn, fnKeyRtn);
  Z_RW_LOCK_INIT(HashTable_t, table, sem);

  return table;
}
void* zHashNodeFind(void *table, void *memb, int *pIndex)
{
  Z_R_LOCK(HashTable_t, table, sem);
  void* ret = _zHashNodeFind(table, memb, pIndex);
  Z_RW_UNLOCK(HashTable_t, table, sem);
  return ret;  
}
int zHashNodeAdd(void * table, void * memb, zFunc_t fnUpdate, int dupCheck)
{
  Z_W_LOCK(HashTable_t, table, sem);
  int ret = _zHashNodeAdd(table, memb, fnUpdate, dupCheck);
  Z_RW_UNLOCK(HashTable_t, table, sem);
  return ret;  
}
int zHashNodeDel(void *table, void *memb)
{
  Z_W_LOCK(HashTable_t, table, sem);
  int ret = _zHashNodeDel(table, memb);
  Z_RW_UNLOCK(HashTable_t, table, sem);
  return ret;  
}
int zHashDestroy(void *table)
{
  Z_W_LOCK(HashTable_t, table, sem);
  int ret = _zHashDestroy(table);
  return ret;  
}

int zHashNodeEach(void *table, zFunc_t fnEach)
{
  Z_R_LOCK(HashTable_t, table, sem);
  int ret = zHashNodeEach(table, fnEach);
  Z_RW_UNLOCK(HashTable_t, table, sem);
  return ret;  
}
int zHashRebuild(void *table)
{
  Z_W_LOCK(HashTable_t, table, sem);
  int ret = _zHashRebuild(table);
  Z_RW_UNLOCK(HashTable_t, table, sem);
  return ret;  
}

int zHashShow(void *table, zFunc_t fnPrint, int flag)
{
  int i;
  HashTable_t *pTable = (HashTable_t *)table;

  zTraceP("---HashShow\n");
  
  zTraceP("sem     : %p\n", pTable->sem);
  zTraceP("fnCmpRtn: %s\n", symFuncNameGet(pTable->fnCmpRtn));
  zTraceP("fnKeyRtn: %s\n", symFuncNameGet(pTable->fnKeyRtn));
  
  zTraceP("log2Size: %d\n", pTable->log2Size);

  if(!flag) return 0;
  for(i=0; i<pTable->log2Size; i++)
  {
    if(pTable->headSlot[i] <= 0) continue;
    zLineLinkShow(&pTable->nodeList, pTable->headSlot[i], fnPrint);
  }
  
  zTraceP("---End\n\n");

  return 0;
}

#define HASH_TEST
#ifdef HASH_TEST
typedef struct TEST_HASH_DATA_TYPE
{
  int a; 
  int b;
  int c;
  byte_t d;
  byte_t e;
  word_t f;
} TestHashDataT;

int testHashCompare(void *memb0, void *memb1)
{
  TestHashDataT *p0 = (TestHashDataT*)memb0;
  TestHashDataT *p1 = (TestHashDataT*)memb1;

  if(p0->a == p1->a)
  {
    if(p0->b == p1->b)
    {
      return p0->c - p1->c;
    }

    return p0->b - p1->b;
  }

  return p0->a - p1->a;
}

int testHashKey(void *memb)
{
  TestHashDataT *p0 = (TestHashDataT*)memb;
  
  return p0->a;
}

int testHashPrint(int nIndex, void *memb, int size)
{
  TestHashDataT *p0 = (TestHashDataT*)memb;

  if(!p0)
  {
    zTraceP("*******************\n");
    return -1;
  }
  
  zTraceP("[%d]   a=%d, b=%d, c=%d, d=%d, e=%d.\n", p0->a, p0->b, p0->c, p0->d, p0->e);

  return 0;
}

int testHash()
{
  void *table = zHashCreate("TEST HASH", 10, sizeof(TestHashDataT), testHashCompare, testHashKey);

  zHashShow(table, 0, 3);

  TestHashDataT x; memset(&x, 0, sizeof (x));

  x.a = 0x11; zHashNodeAdd(table, &x, 0, 0);
  x.a = 0x12; zHashNodeAdd(table, &x, 0, 0);
  x.a = 0x13; zHashNodeAdd(table, &x, 0, 0);
  x.a = 0x14; zHashNodeAdd(table, &x, 0, 0);
  
  x.a = 0x16; zHashNodeAdd(table, &x, 0, 0);
  x.a = 0x21; zHashNodeAdd(table, &x, 0, 0);
  x.a = 0x25; zHashNodeAdd(table, &x, 0, 0);
  x.a = 0x26; zHashNodeAdd(table, &x, 0, 0);

  zHashShow(table, 0, 1);

  zHashRebuild(table);
  zHashShow(table, 0, 1);

  x.a = 0x16; zHashNodeDel(table, &x);
  x.a = 0x11; zHashNodeDel(table, &x);
  x.a = 0x12; zHashNodeDel(table, &x);
  x.a = 0x13; zHashNodeDel(table, &x);
  x.a = 0x14; zHashNodeDel(table, &x);
  x.a = 0x21; zHashNodeDel(table, &x);
  
  zHashShow(table, 0, 1);

  return 0;
}


#endif

