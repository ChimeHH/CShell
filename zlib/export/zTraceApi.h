/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/

#ifndef __TRACE_API_H__
#define __TRACE_API_H__

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif

int z_IamShell();

int z_InShell();
int z_InKernal();
int z_InInterrupt();


extern int z_Log2ConsoleCbkSet(int (*fnPrint)(const char *, int));
extern int z_Log2ZcnCbkSet(int (*fnPrint)(const char *, int));
extern int z_Log2FileCbkSet(int (*fnPrint)(const char *, int));
extern int z_Log2MemoryCbkSet(int (*fnPrint)(const char *, int));

extern int z_ShellLog(const char *fmt, ...) __attribute__((format(printf,1,2)));
extern int z_IntLog  (const char *fmt, ...) __attribute__((format(printf,1,2)));
extern int z_TaskLog (const char *fmt, ...) __attribute__((format(printf,1,2)));

extern int z_ShellLogHex(const byte_t *pData, int nLen);
extern int z_IntLogHex  (const byte_t *pData, int nLen);
extern int z_TaskLogHex (const byte_t *pData, int nLen);

extern int z_ShellPrint(const char *fmt, ...) __attribute__((format(printf,1,2)));
extern int z_ShellPrintHex(const byte_t *pData, int nLen);


int zTraceTaskLevel();

int zTraceBlockOn();
int zTraceBlockOff();
  
int zTraceServiceInit();

int zTraceLevelReset();
int zTraceLevelSet(void* tid, int level);
int zTraceFlagSet(void* tid, int bLogFlag);
int zTraceLevelSetAll(int level);




int zTraceMemoryReset();
int zTraceMemoryInit(int size);
int zTraceMemoryShow(int detail);

/*trace all, except minor and trivil.*/
#define TRACE_ALL_LEVEL 0xFF

/*Any update to below level should update g_strTraceLevelName as well.*/
#define TraceFatal  0x01  /*fatal errors, not recoverable, and is starting reboot soon*/
#define TraceAlarm  0x02  /*emergent errors, not recoverable, need users' maintence check.*/
#define TraceError  0x04  /*software errors*/
#define TraceWarn   0x08  /*configure errors*/
#define TraceInfo   0x10  /*key processes like message traces, for designer, tester's purpose*/
#define TraceDebug  0x20  /*any other verbose session information for designing, debuging intents*/
                          /*0x40 0x80, reserved 2 bits here*/
#define TraceMinor  0x100 /*any other verbose non-session information for designing, debuging intents*/
#define TraceTrivial 0x200 /*any other verbose cycle, audit information for designing, debuging intents*/


#define VERBOSE_LINE(fnPrint, level) fnPrint("[%s]:%s %s %d::", _STR(level), __FILE__, __FUNCTION__, __LINE__)
#define zTraceV(level, fmt, arg...) do{ \
  if(zTraceTaskLevel() & _CONS(Trace,level)) {\
    zTraceBlockOn();  \
    VERBOSE_LINE(z_TaskLog, level); \
    z_TaskLog(fmt, ##arg); \
    zTraceBlockOff();  \
  } \
}while(0)
#define zTraceQ(level, fmt, arg...) do{ \
  if(zTraceTaskLevel() & _CONS(Trace,level)) {\
    zTraceBlockOn(); \
    z_TaskLog(fmt, ##arg); \
    zTraceBlockOff(); \
  } \
}while(0)
#define zTraceP(fmt, arg...) do{ \
  zTraceBlockOn(); \
  z_ShellPrint(fmt, ##arg); \
  zTraceBlockOff(); \
}while(0)
#define zTracePV(fmt, arg...) do{ \
  zTraceBlockOn(); \
  VERBOSE_LINE(z_TaskLog, Console); \
  z_ShellPrint(fmt, ##arg); \
  zTraceBlockOff(); \
}while(0)


#define zTraceHexV(level, pData, nLen) do{ \
  if(zTraceTaskLevel() & _CONS(Trace,level)) {\
    zTraceBlockOn(); \
    VERBOSE_LINE(z_TaskLog, level); \
    z_TaskLogHex((pData), nLen); \
    zTraceBlockOff(); \
  } \
}while(0)
#define zTraceHexQ(level, pData, nLen) do{ \
  if(zTraceTaskLevel() & _CONS(Trace,level)) {\
    zTraceBlockOn(); \
    z_TaskLogHex((pData), nLen); \
    zTraceBlockOff(); \
  } \
}while(0)
#define zTraceHexP(pData, nLen) do{  \
  zTraceBlockOn(); \
  z_ShellPrintHex((byte_t*)(pData), nLen); \
  zTraceBlockOff(); \
}while(0)




#define zTraceFatal(fmt, arg...)    zTraceV(Fatal,  fmt, ##arg)
#define zTraceAlarm(fmt, arg...)    zTraceV(Alarm,  fmt, ##arg)
#define zTraceError(fmt, arg...)    zTraceV(Error,  fmt, ##arg)
#define zTraceWarn( fmt, arg...)    zTraceV(Warn,   fmt, ##arg)
#define zTraceInfo( fmt, arg...)    zTraceV(Info,   fmt, ##arg)
#define zTraceDebug(fmt, arg...)    zTraceV(Debug,  fmt, ##arg)
#define zTraceMinor(fmt, arg...)    zTraceV(Minor,  fmt, ##arg)
#define zTraceTrivial(fmt, arg...)   zTraceV(Trivial,  fmt, ##arg)


#define zTraceFatalQ(fmt, arg...)   zTraceQ(Fatal , fmt, ##arg)
#define zTraceAlarmQ(fmt, arg...)   zTraceQ(Alarm , fmt, ##arg)
#define zTraceErrorQ(fmt, arg...)   zTraceQ(Error , fmt, ##arg)
#define zTraceWarnQ( fmt, arg...)   zTraceQ(Warn  , fmt, ##arg)
#define zTraceInfoQ( fmt, arg...)   zTraceQ(Info,   fmt, ##arg)
#define zTraceDebugQ(fmt, arg...)   zTraceQ(Debug , fmt, ##arg)
#define zTraceMinorQ(fmt, arg...)   zTraceQ(Minor , fmt, ##arg)
#define zTraceTrivialQ(fmt, arg...)  zTraceQ(Trivial , fmt, ##arg)


#define zTraceHexFatal(pData, nLen)    zTraceHexV(Fatal,  (byte_t*)(pData), nLen)
#define zTraceHexAlarm(pData, nLen)    zTraceHexV(Alarm,  (byte_t*)(pData), nLen)
#define zTraceHexError(pData, nLen)    zTraceHexV(Error,  (byte_t*)(pData), nLen)
#define zTraceHexWarn( pData, nLen)    zTraceHexV(Warn,   (byte_t*)(pData), nLen)
#define zTraceHexInfo( pData, nLen)    zTraceHexV(Info,   (byte_t*)(pData), nLen)
#define zTraceHexDebug(pData, nLen)    zTraceHexV(Debug,  (byte_t*)(pData), nLen)
#define zTraceHexMinor(pData, nLen)    zTraceHexV(Minor,  (byte_t*)(pData), nLen)
#define zTraceHexTrivial(pData, nLen)   zTraceHexV(Trivial,  (byte_t*)(pData), nLen)

#define zTraceHexFatalQ(pData, nLen)   zTraceHexQ(Fatal , (byte_t*)(pData), nLen)
#define zTraceHexAlarmQ(pData, nLen)   zTraceHexQ(Alarm , (byte_t*)(pData), nLen)
#define zTraceHexErrorQ(pData, nLen)   zTraceHexQ(Error , (byte_t*)(pData), nLen)
#define zTraceHexWarnQ( pData, nLen)   zTraceHexQ(Warn  , (byte_t*)(pData), nLen)
#define zTraceHexInfoQ( pData, nLen)   zTraceHexQ(Info,   (byte_t*)(pData), nLen)
#define zTraceHexDebugQ(pData, nLen)   zTraceHexQ(Debug , (byte_t*)(pData), nLen)
#define zTraceHexMinorQ(pData, nLen)   zTraceHexQ(Minor , (byte_t*)(pData), nLen)
#define zTraceHexTrivialQ(pData, nLen)  zTraceHexQ(Trivial , (byte_t*)(pData), nLen)






#define PRINT_THREAD_INFO(tid) do{ printf("%s: %d\n", _STR(tid), (int)tid); }while(0)
#define PRINT_THREAD_LIST(tlist, n) do{ \
  int i; \
  for(i=0; i<n; i++) printf("%s[%d]: %d\n", _STR(tlist), i, (int)tlist[i]); \
}while(0)



#ifdef __cplusplus
}
#endif


#endif /*__TRACE_API_H__*/
