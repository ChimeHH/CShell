/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/


#ifndef __Z_HASH_API_H__
#define __Z_HASH_API_H__


#include "zType_Def.h"


#ifdef __cplusplus
extern "C" {
#endif




void *zHashCreate(const char *name, int nmemb, int width, zFunc_t fnCmpRtn, zFunc_t fnKeyRtn);
void *zHashCreate_r(const char *name, int nmemb, int width, zFunc_t fnCmpRtn, zFunc_t fnKeyRtn);

void* zHashNodeFind(void *table, void *memb, int *pIndex);
int zHashNodeAdd(void * table, void * memb, zFunc_t fnUpdate, int dupCheck);
int zHashNodeDel(void *table, void *memb);
int zHashDestroy(void *table);
int zHashNodeEach(void *table, zFunc_t fnEach);
int zHashRebuild(void *table);
int zHashShow(void *table, zFunc_t fnPrint, int flag);




#ifdef __cplusplus
}
#endif

#endif /*__Z_HASH_API_H__*/






