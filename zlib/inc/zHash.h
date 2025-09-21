/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_HASH_H__
#define __Z_HASH_H__







#ifdef __cplusplus
extern "C" {
#endif




typedef struct _Z_QUICK_HASH_TABLE_TYPE_
{
  sem_t   sem;
  
  zFunc_t  fnCmpRtn;
  zFunc_t  fnKeyRtn;
  
  int  log2Size;

  LinearList_t nodeList;

  int   headSlot[1];
} HashTable_t;


#define HASH_SLOT_SIZE   4
#define HASH_RESERVED_SIZE  4


//always reserve 4bytes before pNodePool for magic validation.
#define MAGIC_SET(pTable)       (*((dword_t*)pTable->pNodePool-1) = 0x12345678)
#define MAGIC_VALID(pTable)     (*((dword_t*)pTable->pNodePool-1) == 0x12345678)



#ifdef __cplusplus
}
#endif

#endif /*__Z_HASH_H__*/






