/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/

#ifndef Z_MSG_API_H
#define Z_MSG_API_H

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif



/*
Usually it makes no sense to create a seperate memory pool for queue messages.
Here zMsgServiceInit wil create unless defMemoryPool is null.
Note, Init can be invoked only once; otherwise, it causes leaks, and unpredictable results.
*/
IMPORT void* zMsgServiceInit(int maxRoute, int semKey, int shmKey, int buffSize, int buffCount, int bResetBuff);
IMPORT int zMsgSizeMax(void* serviceCore);

IMPORT int zMsgShowRoute(void* serviceCore, zAddr_t addr);
IMPORT int zMsgShow(void* serviceCore);


typedef int (*AddrCompare_t)(zAddr_t addr0, zAddr_t addr1);
typedef void* (*FuncAddr2Qid_t)(zAddr_t addr);
typedef int   (*FuncAddr2NextHop_t)(zAddr_t addr, dword_t *ip, word_t *port);

IMPORT int zMsgAddRouteEx(void* serviceCore, zAddr_t addr, void* qid, dword_t nextHopIP, word_t nextHopPort, AddrCompare_t fnCompare, FuncAddr2Qid_t fnQid, FuncAddr2NextHop_t fnNextHop);
IMPORT int zMsgAddRoute(void* serviceCore, zAddr_t addr, void *qid, dword_t nextHopIP, word_t nextHopPort);
IMPORT int zMsgDelRoute(void* serviceCore, zAddr_t addr);

typedef int (*Udp2QueMsgHook_t)(zMsgHdr_t *pMsgHdr, dword_t rip, word_t rport); //hooker to udp2que
typedef int (*Que2UdpMsgHook_t)(zMsgHdr_t *pMsgHdr); //hooker to que2udp
IMPORT int zMsgSetHook(void* serviceCore, Udp2QueMsgHook_t hookUdp2Que, Que2UdpMsgHook_t hookQue2Udp);

IMPORT void* zMsgQueCreateEx(const char* name, int maxMsgs, const char* function, int line);

/*
Shall applications call zMemoryAlloc,Free to queue message as to regular buffer? The answer is YES, but only
if msgMemoryPool passed to zMsgServiceInit() is exactly same to the regular memory pool.

Hence, it's strongly recommended that applications to call NEW_MSG, DEL_MSG, and PTR_MSG; that will help
them avoid possible tricks.
*/
IMPORT zHandle_t zMsgNewBuff(void* serviceCore, zMsgHdr_t** ppMsg, const char * function,int line);
IMPORT int       zMsgFreeBuff(void* serviceCore, zHandle_t msgHandle, const char * function,int line);
IMPORT zMsgHdr_t* zMsgMapBuff(void* serviceCore, zHandle_t msgHandle, int *pLen);
#define NEW_MSG(serviceCore, ppMsg)     zMsgNewBuff(serviceCore, ppMsg, __FUNCTION__, __LINE__)
#define DEL_MSG(serviceCore, msgHandle)         zMsgFreeBuff(serviceCore, msgHandle, __FUNCTION__, __LINE__)
#define PTR_MSG(serviceCore, msgHandle, pLen)   zMsgMapBuff(serviceCore, msgHandle, pLen)

IMPORT int zMsgQueRecvEx(void* qid, zMsgHdr_t* pMsg, const char* function, int line); //RETURN: 0 no messages.
IMPORT int zMsgQueSendEx(void* qid, zMsgHdr_t* pMsg, const char* function, int line); //RETURN: <0 on fail.
IMPORT int zMsgSendEx(void* serviceCore, zMsgHdr_t* pMsg, const char* function, int line);
#define zMsgSend(serviceCore, pMsg) zMsgSendEx(serviceCore, pMsg, __FUNCTION__, __LINE__)


IMPORT int zMsgQue2Udp(void *serviceCore, void* qid, int sock, int *errNo, const char* function, int line);
IMPORT int zMsgUdp2Que(void *serviceCore, int sock, int *errNo, const char* function, int line);


#ifdef __cplusplus
}
#endif


#endif /*Z_MSG_API_H*/
