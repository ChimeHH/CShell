/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/


#ifndef __Z_LIST_API_H__
#define __Z_LIST_API_H__


#include "zType_Def.h"


#ifdef __cplusplus
extern "C" {
#endif



int zLineListInit(void *list);
void* zLineListCreate(const char *name, int nmemb, int width, int bReentrant);
int zLineListAlloc(void *list, byte_t **ppBlk);
int zLineListFree(void *list, int nIndex);
void* zLineListData(void *list, int nIndex);

int zLineListLink(void *list, int nIndex, int *pSlot);
int zLineListUnlink(void *list, int *pSlot);
int zLineListShow(void *list, zFunc_t fnEach);
int zLineLinkShow(void *list, int nSlot, zFunc_t fnEach);
int zLineListRebuild(void *list);


void* zSetInit(void* list);
int zSetAdd(void* set, void* data, int len); //return nIndex
int zSetDel(void* set, int nIndex);

int zSetIter(void* set, int offset);
int zSetNext(void* set);


#ifdef __cplusplus
}
#endif

#endif /*__Z_LIST_API_H__*/






