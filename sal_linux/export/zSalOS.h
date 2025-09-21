/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/

#ifndef Z_SAL_OS_H
#define Z_SAL_OS_H

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <string.h>

extern char *strcasestr(const char *haystack, const char *needle);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "zType_Def.h"

#include <semaphore.h>


#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************
                          MUTEX LOCK
*********************************************************************/
void *zMutexInit();
int zMutexLock(void *mutex);
int zMutexTryLock(void *mutex);
int zMutexUnlock(void *mutex);
int zMutexDestory(void *mutex);


/********************************************************************
                               SPIN LOCK
*********************************************************************/
volatile void *zSpinInit();
int zSpinLock(void *spin);
int zSpinTryLock(void *spin);
int zSpinUnlock(void *spin);
int zSpinDestory(void *spin);


/********************************************************************
                     READ & WRITE LOCK
*********************************************************************/
void *zRwLockInit();
int zRwLockRdLock(void *rw);
int zRwLockWrLock(void *rw);
int zRwLockTryRdLock(void *rw);
int zRwLockTryWrLock(void *rw);
int zRwLockTimedRdLock(void *rw, int sec, int nsec);
int zRwLockTimedWrLock(void *rw, int sec, int nsec);
int zRwLockUnlock(void *rw);
int zRwLockDestory(void *rw);

#define Z_RW_LOCK_INIT(stype, p, sem) do{ /*if(p) ((stype*)p)->sem = zRwLockInit();*/}while(0)
#define Z_R_LOCK(stype, p, sem)       do{ /*if(p) zRwLockRdLock(((stype*)p)->sem); */}while(0)
#define Z_W_LOCK(stype, p, sem)       do{ /*if(p) zRwLockWrLock(((stype*)p)->sem); */}while(0)
#define Z_RW_UNLOCK(stype, p, sem)    do{ /*if(p) zRwLockUnlock(((stype*)p)->sem); */}while(0)

int zSemInit(int key, int *bExist);
int zSemLock(int sem);
int zSemUnlock(int sem);
int zSemDestroy(int sem);


/********************************************************************
                     TASK (wrap pthread)
*********************************************************************/

typedef struct TASK_INFO_TYPE
{
  void* task;
  dword_t hbCount;
} zTaskInfo_t;

void* zTaskCreateEx(const char* taskName, void *(*fnEntry) (void *), void* arg);
#define zTaskCreate(entry, arg) zTaskCreateEx(_STR(entry), entry, arg)

void* zTaskSelf();
int zTaskEqual(void* tid1, void* tid2);
int zTaskStop(void* tid);

const char* zTaskId2Name(void* tid);
void* zTaskIdEx(const char* name, void* entry);
int zTaskList();

/********************************************************************
            RAW Trace (applications shall use zTrace instead.)
*********************************************************************/
int z_Log2ConsoleCbkSet(int (*fnPrint)(const char *, int));
int z_Log2ZcnCbkSet(int (*fnPrint)(const char *, int));
int z_Log2FileCbkSet(int (*fnPrint)(const char *, int));
int z_Log2MemoryCbkSet(int (*fnPrint)(const char *, int));

int z_ShellLog(const char *fmt, ...);
int z_IntLog  (const char *fmt, ...);
int z_TaskLog (const char *fmt, ...);
int z_ShellLogHex(const byte_t *pData, int nLen);
int z_IntLogHex  (const byte_t *pData, int nLen);
int z_TaskLogHex (const byte_t *pData, int nLen);
int z_ShellPrint(const char *fmt, ...);
int z_ShellPrintHex(const byte_t *pData, int nLen);

typedef int (*XPRINT_CALL)();

/********************************************************************
            RAW Socket (applications shall use zTrace instead.)
*********************************************************************/
typedef struct SOCKET_EX_TYPE
{ 
  int sock; 
  dword_t readable:1; 
  dword_t writable:1;
  dword_t exception:1;
  dword_t rsvd:29; 
} zSockEx_t;
//returns 0 on error
dword_t zIPAddr(const char *cp); //host byte-order
dword_t zInetAddr(const char *cp); //net byte-order

IMPORT int zCreateSocketEx(word_t lport, const char* function, int line);
IMPORT int zSendtoEx(int sock, dword_t rip, word_t rport, byte_t *buf, int len, const char* function, int line);
IMPORT int zTryRecvfromEx(int sock, dword_t *rip, word_t *rport, byte_t *buf, int len, int sec, int usec, const char* function, int line);
IMPORT int zEnumateSocketEx(int sec, int usec, zSockEx_t ex[], int count, int *nread, int *nwrite, int *nexception, const char* function, int line);
IMPORT int zRecvfromEx(int sock, dword_t *rip, word_t *rport, byte_t *buf, int len, const char* function, int line);
IMPORT int zShutdownSocketEx(int sock, const char* function, int line);
IMPORT int zCloseSocketEx(int sock, const char* function, int line);

IMPORT int zCanReadSocketEx(int sock, const char* function, int line);
IMPORT int zCanWriteSocketEx(int sock, const char* function, int line);

#define zCreateSocket(lport) \
      zCreateSocketEx(lport, __FUNCTION__, __LINE__)
#define zSendto(sock, rmtIP, rmtPort, pBuf, len) \
      zSendtoEx(sock, rmtIP, rmtPort, (byte_t*)(pBuf), len, __FUNCTION__, __LINE__)
#define zTryRecvfrom(sock, pRmtIP, pRmtPort, pBuf, len, sec, usec) \
      zTryRecvfromEx(sock, pRmtIP, pRmtPort, (byte_t*)(pBuf), len, sec, usec, __FUNCTION__, __LINE__)
#define zEnumateSocket(sec, usec, pSockEx, sockNum, pRead, pWrite, pExcept) \
      zEnumateSocketEx(sec, usec, pSockEx, sockNum, pRead, pWrite, pExcept, __FUNCTION__, __LINE__)
#define zRecvfrom(sock, pRmtIP, pRmtPort, pBuf, len) \
      zRecvfromEx(sock, pRmtIP, pRmtPort, (byte_t*)(pBuf), len, __FUNCTION__, __LINE__)
#define zShutdownSocket(sock) \
      zShutdownSocketEx(sock, __FUNCTION__, __LINE__)
#define zCloseSocket(sock) \
      zCloseSocketEx(sock, __FUNCTION__, __LINE__)
#define zCanReadSocket(sock) \
      zCanReadSocketEx(sock, __FUNCTION__, __LINE__)
#define zCanWriteSocket(sock) \
      zCanWriteSocketEx(sock, __FUNCTION__, __LINE__)

int zSleep(int sec);
int zSleepUSec(int usec);
int zTime();
char *zCTime(int *pT);

int zIpcStr2Key(const char* name);

void* zShmGetEx(int key, int maxSize, int *shmId, int *bExist, const char* function, int line);
#define zShmGet(key, maxSize, pShmId, pExist) zShmGetEx(key, maxSize, pShmId, pExist, __FUNCTION__, __LINE__)
int zShmSize(int id);
int zShmAt(int id, void* addr);
int zShmDt(int id);
int zShmDistory(int id);


/**
redefine
**/
#define zMemset memset

#ifdef __cplusplus
}
#endif

#endif /*Z_SAL_OS_H*/

