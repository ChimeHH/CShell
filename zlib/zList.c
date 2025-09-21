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
#include "zListApi.h"

static int NodeDataEachDefault(int nIndex, void *data, int size)
{
  ListNode_t *pNode = NODE_FROM_DATA(data);
  
  zTraceP("{ %8d -> %8d, %d : ", nIndex, pNode->next, pNode->bUsed);
  zTraceHexP(pNode->data, size);
  zTraceP(" }\n");

  return 0;
}



int zLineListInit(void *list)
{
  int i;

  LinearList_t *pList = (LinearList_t *)list;

  if(!pList)
  {
    zTraceError("Invalid null list input.\n");
    return -1;
  }

  if(!pList->pNodePool || pList->maxCount<=0 || pList->dataSize<=0)
  {
    zTraceError("Invalid maxCount %d or pool %p or dataSize %d.\n", pList->maxCount, pList->pNodePool, pList->dataSize);
    return -2;
  }

  for(i=1; i<pList->maxCount-1; i++)
  {
    LIST_NODE_USED(pList, i) = 0;
    LIST_NODE_NEXT(pList, i) =  i+1;
  }

  LIST_NODE_USED(pList, pList->maxCount-1) = 0;
  LIST_NODE_NEXT(pList, pList->maxCount-1) = 0;

  pList->freeHead = 1;
  pList->freeCount = pList->maxCount - 1; //reserve the first 0 slot.


  return 0;
}

void* zLineListCreate(const char *name, int nmemb, int width, int bReentrant)
{
  int size;
  LinearList_t *pList;

  size = sizeof(LinearList_t) + nmemb * NODE_SIZE(width);
  pList = (LinearList_t *)malloc(size);
  if(!pList)
  {
    zTraceFatal("heap alloc failed\n");
    return 0;
  }
  memset(pList, 0, size);
  pList->pNodePool = (void*)(pList+1);
  
  pList->maxCount = nmemb;
  pList->dataSize = width;

  
  if(bReentrant) pList->lock = zMutexInit();

  zLineListInit(pList);
  
  return pList;
}

int zLineListAlloc(void *list, byte_t **ppData)
{
  int ret = 0;
  LinearList_t *pList = (LinearList_t *)list;

  if(pList->lock) zMutexLock(pList->lock);
  
  if(pList->freeHead > 0)
  {
    ret = pList->freeHead;
    pList->freeHead = LIST_NODE_NEXT(pList, ret);
    LIST_NODE_USED(pList, ret) = 1;
    LIST_NODE_NEXT(pList, ret) = 0;
    if(ppData) *ppData = LIST_NODE_DATA(pList, ret);
    pList->freeCount -= 1;
  }
  
  if(pList->lock) zMutexUnlock(pList->lock);
  
  return ret;
}

int zLineListFree(void *list, int nIndex)
{
  LinearList_t *pList = (LinearList_t *)list;

  if( nIndex <=0 || nIndex >=pList->maxCount || !LIST_NODE_USED(pList,nIndex) )
  {
    zTraceError("Invalid List %p nIndex %d\n", list, nIndex);
    return -1;
  }

  if(pList->lock) zMutexLock(pList->lock);
  
  LIST_NODE_NEXT(pList, nIndex) = pList->freeHead;
  LIST_NODE_USED(pList, nIndex) = 0;
  pList->freeCount += 1;
  pList->freeHead = nIndex;
  
  if(pList->lock) zMutexUnlock(pList->lock);

  return 0;
}

void* zLineListData(void *list, int nIndex)
{
  LinearList_t *pList = (LinearList_t *)list;

  if( nIndex <=0 || nIndex >=pList->maxCount || !LIST_NODE_USED(pList,nIndex) )
  {
    zTraceError("Invalid List %p nIndex %d\n", list, nIndex);
    return 0;
  }

  
  return LIST_NODE_DATA(pList, nIndex);  
}


int zLineListLink(void *list, int nIndex, int *pSlot)
{
  LinearList_t *pList = (LinearList_t *)list;

  if( nIndex <=0 || nIndex >=pList->maxCount || !LIST_NODE_USED(pList,nIndex) || !pSlot)
  {
    zTraceError("Invalid List %p nIndex %d, Slot %p\n", list, nIndex, pSlot);
    return -1;
  }

  if(pList->lock) zMutexLock(pList->lock);
  
  LIST_NODE_NEXT(pList, nIndex) = *pSlot;
  *pSlot = nIndex;

  if(pList->lock) zMutexUnlock(pList->lock);

  return 0;
}

int zLineListUnlink(void *list, int *pSlot)
{
  LinearList_t *pList = (LinearList_t *)list;
  if(!pSlot)
  {
    zTraceError("Invalid List %p, Slot %p\n", list, pSlot);
    return -1;
  }

  if(*pSlot <=0 || *pSlot >=pList->maxCount || !LIST_NODE_USED(pList,*pSlot) )
  {
    zTraceError("Invalid List %p, Slot %p %d\n", list, pSlot, *pSlot);
    return -2;
  }

  if(pList->lock) zMutexLock(pList->lock);
  
  {
    int nTemp = *pSlot;
    *pSlot = LIST_NODE_NEXT(pList, nTemp);
    LIST_NODE_NEXT(pList, nTemp) = 0;
  }

  if(pList->lock) zMutexUnlock(pList->lock);

  return 0;
}


int zLineListShow(void *list, zFunc_t fnEach)
{
  int i;
  LinearList_t *pList = (LinearList_t *)list;
  if(!list)
  {
    zTraceFatal("invalid list %p\n", list);
    return 0;
  }

  zTraceP("%s:\n", __FUNCTION__);

  if(!fnEach) fnEach = (zFunc_t)NodeDataEachDefault;

  zTraceP("list.maxCount:  %d\n", pList->maxCount);
  zTraceP("list.dataSize:  %d\n", pList->dataSize);
  zTraceP("list.freeCount: %d\n", pList->freeCount);
  zTraceP("list.freeHead:  %d\n", pList->freeHead);
  zTraceP("list.pNodePool: %p\n", pList->pNodePool);

  //if(pList->lock) zMutexLock(pList->lock);
  
  for(i=0; i<pList->maxCount; i++)
  {
    fnEach(i, LIST_NODE_DATA(pList, i), LIST_DATA_SIZE(pList));
  }
  
  //if(pList->lock) zMutexUnlock(pList->lock);

  return 0;
}

int zLineLinkShow(void *list, int nSlot, zFunc_t fnEach)
{  
  LinearList_t *pList = (LinearList_t *)list;
  if(!list)
  {
    zTraceFatal("invalid list %p\n", list);
    return 0;
  }

  if(!fnEach) fnEach = (zFunc_t)NodeDataEachDefault;
  zTraceP("%s:\n", __FUNCTION__);
  while(nSlot > 0)
  {
    fnEach(nSlot, LIST_NODE_DATA(pList, nSlot), LIST_DATA_SIZE(pList));
    nSlot = LIST_NODE_NEXT(pList, nSlot);
  }
  
  return 0;
}

int zLineListRebuild(void *list)
{
  int i;
  LinearList_t *pList = (LinearList_t *)list;
  if(!list)
  {
    zTraceFatal("invalid list %p\n", list);
    return 0;
  }

  pList->freeHead = 0;
  pList->freeCount = 0;

  if(pList->lock) zMutexLock(pList->lock);
  
  for(i=1; i<pList->maxCount; i++) //reserve the first slot 0. see ListCreate()
  {
    ListNode_t *pNode = LIST_NODE_PTR(pList, i);
    if(NODE_USED(pNode)) continue;

    pList->freeCount += 1;
    NODE_NEXT(pNode) = pList->freeHead;
    pList->freeHead = i;
  }

  if(pList->lock) zMutexUnlock(pList->lock);
  
  return 0;
}


#define LIST_TEST
#ifdef LIST_TEST
int testLineList()
{
  int ret;
  void *list = zLineListCreate("TEST LIST", 10, 4, 1);

  int out[2] = {0, 0};

  zLineListShow(list, 0);

  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[0]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[0]);  
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[0]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[0]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[0]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);

  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret); zLineListLink(list, ret, &out[1]);

  zLineListShow(list, 0);
  zLineLinkShow(list, out[0], 0);
  zLineLinkShow(list, out[1], 0);

  ret = LIST_NODE_NEXT((LinearList_t*)list, out[0]);
  zLineListUnlink(list, &LIST_NODE_NEXT((LinearList_t*)list, out[0]));
  zLineListFree(list, ret);
  ret = out[1];
  zLineListUnlink(list, &out[1]);
  zLineListFree(list, ret);
  zLineListShow(list, 0);
  zLineLinkShow(list, out[0], 0);
  zLineLinkShow(list, out[1], 0);

  zTracePV("free....\n");
  zLineListFree(list, 1);
  zLineListFree(list, 3);
  zLineListFree(list, 5);
  zLineListFree(list, 6);
  zLineListFree(list, 7);
  zLineListFree(list, 8);
  zLineListFree(list, 2);
  zLineListFree(list, 9);
  zLineListFree(list, 0);
  zLineListFree(list, 4);

  zLineListFree(list, 4);
  zLineListFree(list, 9);

  zLineListShow(list, 0);

  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret);
  ret = zLineListAlloc(list, 0); zTracePV("alloc %d\n", ret);  
  zLineListShow(list, 0);

  return 0;
}

#endif

