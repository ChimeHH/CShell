/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef MEM_LEAK_H
#define MEM_LEAK_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct MEMORY_LEAK_TYPE
{
  void* mem;
  int   size;
  const char* function;
  int   line;
  int   tAlloc;
} MemLeak_t;




#ifdef __cplusplus
}
#endif


#endif /*MEM_LEAK_H*/
