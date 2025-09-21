/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_TRACE_H__
#define __Z_TRACE_H__

#ifdef __cplusplus
extern "C" {
#endif



typedef struct _Z_TRACE_STAT_TYPE_
{
  dword_t dwFatal;
  dword_t dwAlarm;
  dword_t dwError;
  dword_t dwWarn;
  dword_t dwNotify;
} ZTraceStatT;






typedef struct _Z_TRACE_NODAL_TYPE_
{
  void*  tid;
  byte_t bLogFlag:1;
  int    level;
} ZTraceNodalT;

typedef struct _Z_TRACE_CTRL_TYPE_
{
  int enable;
  int count;
  ZTraceNodalT nodal[4];
} ZTracCtrlT;




#ifdef __cplusplus
}
#endif


#endif /*__Z_TRACE_H__*/
