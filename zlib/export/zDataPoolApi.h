/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/


#ifndef Z_DATAPOOL_API_H
#define Z_DATAPOOL_API_H


#include "zType_Def.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*FunDataPrintEach_t)(int nIndex, int bUsedFlag, void* pInst, int dataSize, int bForceFlag);
void* DataPoolCreateEx(const char* name, FunDataPrintEach_t fnPrintEach, int maxCount, int dataSize, int bReentrant, int bHA, const char* function, int line);

//Note: even data pool is using share memory, it can't be read by more than 1 process, because the sem is not available by multiprocesses.
//      Here share memory is intent to meet HA requirement
#define DataPoolCreate(name, fnPrintEach, maxCount, dataSize, bReentrant) DataPoolCreateEx(name, fnPrintEach, maxCount, dataSize, bReentrant, 0, __FUNCTION__, __LINE__)
#define DataPoolCreateShm(name, fnPrintEach, maxCount, dataSize, bReentrant) DataPoolCreateEx(name, fnPrintEach, maxCount, dataSize, bReentrant, 1, __FUNCTION__, __LINE__)

int   DataPoolDestory(void* pool);

int   DataBlockAudit(void* pool, int bSet);
int   DataBlockMaxId(void* pool);
int   DataBlockFreeCount(void* pool);

int   DataBlockAlloc(void* pool, void *ppInstance);
int   DataBlockFree(void* pool, int nIndex);
int DataBlockSetBusy(void* pool, int nIndex);
void *DataBlockMap(void* pool, int nIndex);

void *DataBlockMapEx(void* pool, int nIndex, int *bUsedFlag);

//fnEach(nIndex, bUsedFlag, void* pInstance, int dataSize, int userData)
int   DataPoolWalk(void* pool, zFunc_t fnEach, int userData);
int   DataPoolShow(void* pool, int bForceFlag);



#ifdef __cplusplus
}
#endif

#endif /*Z_DATAPOOL_API_H*/






