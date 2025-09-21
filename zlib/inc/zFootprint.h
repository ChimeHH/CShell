/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef FOOTPRINT_H
#define FOOTPRINT_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ERROR_NO_TYPE
{
  const char* errstr;
  dword_t     param0;
  dword_t     param1;
  
  const char* function;
  int     line;
  int     count;
  int     when;
} PACKED Eos_t;


typedef struct FOOTPRINT_TYPE
{
  const char* errstr;
  dword_t  func:20; //symFuncNameEval(addr);
  dword_t  line:12;
} PACKED Footprint_t;



typedef struct FOOTPRINT_SERVICE_TYPE
{
  dword_t maxCount;
  void*   lock;
  
  dword_t next;
  Footprint_t fp[1];
} FpService_t;



#ifdef __cplusplus
}
#endif

#endif /*FOOTPRINT_H*/






