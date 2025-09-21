/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/


#ifndef __Z_MEMORY_API_H__
#define __Z_MEMORY_API_H__


#include "zType_Def.h"


#ifdef __cplusplus
extern "C" {
#endif



void *zMemoryPoolCreate(const char* name, dword_t size, int bReentrant);

int zMemoryAlloc(void *pool, int size, void **ppMem, const char* function, int line);

int zMemoryFreeEx(void *pool, int nMemId, int bAuth);
int zMemoryFree(void *pool, int nMemId);

int zMemoryDownsizing(void *pool, int nMemId, int newSize);

void *zMemoryMap(void *pool, int nMemId, int *pLen);

int zMemoryPoolShow(void *pool, int bStat);


#ifdef __cplusplus
}
#endif

#endif /*__Z_MEMORY_API_H__*/






