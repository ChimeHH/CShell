/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef Z_TYPE_DEF_H
#define Z_TYPE_DEF_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <arpa/inet.h>

//#define printf not_defined_printf

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FAST
#define FAST   register
#endif

#ifndef IMPORT
#define IMPORT extern
#endif

#ifndef LOCAL
#define LOCAL  static
#endif

#ifndef NOT_USED_VAR
#define NOT_USED_VAR(var) (void)(var)
#endif  

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   1
#endif

#ifndef PACKED
#define PACKED __attribute__ ((packed))
#endif

#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a, b) ((a)<(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(a)   ((a)>0?(a):-(a))
#endif

#ifndef SWAP
#define SWAP(type, a, b) do { type x = b; b = a; a = x; }while(0)
#endif




#if (_BYTE_ORDER == _LITTLE_ENDIAN)
  #define IS_LITTLE_ENDIAN
  
  #define M32_MAGIC_VALUE   0x01323232
  #define M64_MAGIC_VALUE   0x01646464
#else
  #define IS_BIG_ENDIAN

  #define M32_MAGIC_VALUE   0x02323232
  #define M64_MAGIC_VALUE   0x02646464
#endif /* (_BYTE_ORDER == _LITTLE_ENDIAN) */

#define MAGIC_VALUE ((sizeof(void*)==8) ? M64_MAGIC_VALUE : M32_MAGIC_VALUE)


#ifndef _DEF_BYTE_
  #define _DEF_BYTE_
  typedef u_int8_t    byte_t;  /*8 bits*/
#endif

#ifndef _DEF_WORD_
  #define _DEF_WORD_
  typedef u_int16_t   word_t;  /*16 bits*/
#endif

#ifndef _DEF_DWORD_
  #define _DEF_DWORD_
  typedef u_int32_t   dword_t; /*32 bits*/
#endif

#ifndef _DEF_LWORD_
  #define _DEF_LWORD_
  typedef u_int64_t   lword_t; /*64 bits*/
#endif

#ifndef _DEF_HANDLE_
  #define _DEF_HANDLE_
  typedef int        zHandle_t;
#endif

#ifndef _DEF_FUNC_
  #define _DEF_FUNC_
  typedef int (*zFunc_t)();
#endif



typedef dword_t zAddr_t; /*zone8 + cluster12 + node12*/
#define zAddrZone(addr)    (((dword_t)addr)>>24)
#define zAddrCluster(addr) ((((dword_t)addr)>>12)&0xFFF)
#define zAddrNode(addr)    (((dword_t)addr)&0xFFF)

#define IS_LOCAL_MSG(wMsgId)     ((wMsgId) & 0x8000)
#define LOCAL_EVENT2MSG(wEvent)  ((wEvent) | 0x8000)
#define LOCAL_MSG2EVENT(wMsgId)  ((wMsgId) & 0x7FFF)

/*
Note:
srcAddr zone == 0: local user defined msg. Only dstAddr is generally defined. 
This is intend to fast up the timer, and other short message operations.
for example, in a timer message (advised format, depends on user timecall):
  msgHandle(bit0~15)  => msgName;
  msgHandle(bit16~31) => dstInst;
  srcAddr.cluster/node => nTimer(partial, bit0~23);
*/
typedef struct _MSG_HEAD_TYPE_
{
  //zMsg_t begins. Must keep below items same to zMsg_t, and don't change them.
  zAddr_t srcAddr;
  zAddr_t dstAddr; 
  zHandle_t msgHandle;
  //zMsg_t ends
  
  word_t  msgLen;
  word_t  msgId;  
  dword_t srcInst;
  dword_t dstInst;
} PACKED zMsgHdr_t;

typedef struct _Z_TIME_SPEC_TYPE_
{
  int sec;
  int msec;
} ZTimeSpecT;

typedef struct ENUM2NAME_MAP_T
{
  word_t e;
  char*  name;  
} zEnum2NameMap_t;


#define _STR(s)       #s
#define _CONS(a, b)   a##b

#define TBL_SIZE(array) (sizeof(array)/sizeof(array[0]))

#define PACKSIZE(size)   (((size+1)>>1)<<1)  /*depends on CPU arch and pack(x) options*/


#define COPY_POINTER(pd, ps) do{ \
  (void*)(pd) = (void*)(ps); \
}while(0)
#define COPY_DWORD(pd, ps) do{ \
  *(dword_t)(pd) = *(dword_t)(ps); \
}while(0)
#define COPY_WORD(pd, ps) do{ \
  *(word_t)(pd) = *(word_t)(ps); \
}while(0)

#define MAXINT()    (0x7FFFFFFF)
#define MAXUINT()   (0xFFFFFFFF)


#define PTR_VALUE(t, p) (*(t*)(p))
#define PV2DWORD(p) PTR_VALUE(dword_t, p)
#define PV2WORD(p)  PTR_VALUE(word_t, p)
#define PV2BYTE(p)  PTR_VALUE(byte_t, p)


#define ROUNDSIZE(n)  ((n+sizeof(long)-1)>>2<<2)


#define STRUCT_MEMBER_OFFSET(type, mem) ((void*)&((type*)0)->mem - (void*)0)
#define STRUCT_MEMBER_SIZE(type, mem)  sizeof(((type*)0)->mem)

#define BYTE_OFFSET_PTR(p, offset)    ((byte_t*)(p)+offset)
#define BYTE_OFFSET_VALUE(p, offset)  (*BYTE_OFFSET_PTR(p, offset))
#define BYTE_OFFSET_LS_VALUE(p, offset, shift) (BYTE_OFFSET_VALUE(p, offset) << shift)

#define BYTES2VALUE(p, n) (n==1) ? BYTE_OFFSET_LS_VALUE(p, 0, 0) : \
  (n==2) ? BYTE_OFFSET_LS_VALUE(p, 0, 8) + BYTE_OFFSET_LS_VALUE(p, 1, 0) : \
  (n==3) ? BYTE_OFFSET_LS_VALUE(p, 0, 16) + BYTE_OFFSET_LS_VALUE(p, 1, 8) + BYTE_OFFSET_LS_VALUE(p, 2, 0) : \
  (n==4) ? BYTE_OFFSET_LS_VALUE(p, 0, 24) + BYTE_OFFSET_LS_VALUE(p, 1, 16) + BYTE_OFFSET_LS_VALUE(p, 2, 8) + BYTE_OFFSET_LS_VALUE(p, 3, 0) : \
  -1

#define BYTES2WORD(a, b)   (((a)<<8)|(b))
#define WORDS2DWORD(a, b)  (((a)<<16)|(b))
#define BYTES2DWORD(a, b, c, d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))


#define WORD2BYTES(w, a, b)        do{ a = ((w)>>8)&0xff; b = (w)&0xff; }while(0)
#define DWORD2BYTES(w, a, b, c, d) do{ a = ((w)>>24)&0xff; b = ((w)>>16)&0xff; c = ((w)>>8)&0xff; d = (w)&0xff; }while(0)

#ifndef EOS_SET
#define EOS_SET(eos) do{ /*uncomment for testing overrides*/ }while(0)
#endif
#ifndef EOS_SET_WP
#define EOS_SET_WP(eos, param0, param1) do{ /*uncomment for testing overrides*/ }while(0)
#endif
  
#define TRACE_MEMORY_LEAK 1
#if (TRACE_MEMORY_LEAK==1)
#define malloc(size) tml_malloc(size, __FUNCTION__, __LINE__)
#define realloc(mem, size) tml_malloc(mem, size, __FUNCTION__, __LINE__)
#define free(mem)      tml_free(mem)
IMPORT void* tml_malloc(int size, const char* function, int line);
IMPORT void  tml_free(void* mem);
IMPORT void* tml_realloc(void* mem, int size, const char* function, int line);
#endif



#ifdef __cplusplus
}
#endif

#endif /*Z_TYPE_DEF_H*/

