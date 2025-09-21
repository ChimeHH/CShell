/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_LIST_H__
#define __Z_LIST_H__







#ifdef __cplusplus
extern "C" {
#endif


typedef struct _Z_LIST_NODE_TYPE_
{
  int next;  
  
  dword_t resv:15;
  dword_t bUsed:1;  
  dword_t len:16;
  
  byte_t  data[1];
} PACKED ListNode_t;

#define NODE_HEAD_SIZE          8
#define NODE_SIZE(dataSize)     ( ((dataSize+3)>>2<<2) + NODE_HEAD_SIZE )

#define NODE_NEXT(pNode)        ( (pNode)->next )
#define NODE_USED(pNode)        ( (pNode)->bUsed )
#define NODE_DATA(pNode)        ( (pNode)->data )

#define NODE_FROM_DATA(pData)   ((ListNode_t *)(pData - NODE_HEAD_SIZE))


typedef struct Z_LINEAR_LIST_TYPE
{
  void*  lock;
  int maxCount;  
  int dataSize;

  int freeCount;
  int freeHead;

  ListNode_t *pNodePool;
} LinearList_t;

#define LIST_DATA_SIZE(pList)       ( (pList)->dataSize )
#define LIST_NODE_SIZE(pList)       ( NODE_SIZE( (pList)->dataSize ) )

#define LIST_NODE_PTR(pList, nIndex)  ( (ListNode_t *)( (void*)(pList)->pNodePool + LIST_NODE_SIZE(pList)*(nIndex) ) )
#define LIST_NODE_USED(pList, nIndex) NODE_USED( LIST_NODE_PTR(pList, nIndex) )
#define LIST_NODE_DATA(pList, nIndex) NODE_DATA( LIST_NODE_PTR(pList, nIndex) )
#define LIST_NODE_NEXT(pList, nIndex) NODE_NEXT( LIST_NODE_PTR(pList, nIndex) )

#define LIST_NODE_INDEX(pList, pNode) ( ((void*)(pNode) - (void*)(pList)->pNodePool)/LIST_NODE_SIZE(pList) )



#ifdef __cplusplus
}
#endif

#endif /*__Z_LIST_H__*/






