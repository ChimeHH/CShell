/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <stdio.h> 
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <errno.h>

#include "zType_Def.h"
#include "zSalOS.h"
#include "zTraceApi.h"
#include "zFootprintApi.h"

int zIpcStr2Key(const char* name)
{
  if(!name) return 0;

  int len = strlen(name);

  int key = 0;

  int i;
  for(i=0; i<len; i++)
  {
    key = (key << 8) + name[i] + 1;
    key = (key >> 16) + (key & 0xFFFF);
  }

  return key;
}

void* zShmGetEx(int key, int maxSize, int *shmId, int *bExist, const char* function, int line) 
{
  int id; 
  char* memory; 

  if(bExist) *bExist = 0;
  /* Allocate a shared memory segment.  */ 
  id = shmget ((key_t)key, maxSize, IPC_CREAT | IPC_EXCL | 777); //S_IRUSR | S_IWUSR); //IPC_EXCL to prompt file exists
  if(id < 0)
  {
    EOS_SET(shmget-mem-exist-remap);
    
    id = shmget ((key_t)key, 0, IPC_CREAT | 777); //S_IRUSR | S_IWUSR);
    if(id < 0)
    {
      EOS_SET(shmget-mem-remap-failed);
      
      zTraceAlarm("@%s:%d Failed to create share memory, key=%08x, maxSize=%d, err:%d %s, return: %d\n",
                   function, line, key, maxSize, errno, strerror(errno), id);
      return 0;
    }

    if(bExist) *bExist = 1;
  }

  if(shmId) *shmId = id;

  /* Attach the shared memory segment.  */ 
  memory = (char*) shmat (id, 0, 0);

  zTraceDebug("@%s:%d key: %08x, maxSize: %d, return id: %d.\n", function, line, key, maxSize, id);
  
  return memory;
}

int zShmSize(int id)
{  
  struct shmid_ds shmbuffer; 

  if(id < 0) return -1;
  
  int ret; 
  
  /* Determine the segment's size. */ 
  ret = shmctl (id, IPC_STAT, &shmbuffer);
  if(ret < 0)
  {
    zTraceAlarm("Failed to get share memory size, id=%d\n", id);
    return -1;
  }
  
  return shmbuffer.shm_segsz;
}

int zShmAt(int id, void* addr)
{
  char* memory;

  if(id < 0) return -1;
  
  /* Reattach the shared memory segment, at a different address.  */ 
  memory = (char*) shmat (id, addr, 0);   
  if(!memory)
  {
    zTraceAlarm("Failed to attach memory to %p\n", addr);
    return -1;
  }

  return 0;
}

int zShmDt(int id)
{  
  char* memory;

  if(id < 0) return -1;
  
  /* Print out the string from shared memory.  */
  memory = (char*) shmat (id, 0, 0); 
  
  /* Detach the shared memory segment.  */ 
  if(memory) shmdt (memory); 

  return 0;
}

int zShmDistory(int id)
{
  if(id < 0) return -1;
  
  /* Deallocate the shared memory segment.  */ 
  shmctl (id, IPC_RMID, 0);

  return 0;
}

#define SHM_SELFTEST

#ifdef SHM_SELFTEST

int testShm()
{
  int key = 123;

  int bExist = 0;

  int id1 = 0;
  void* m1 = zShmGet(key, 2000, &id1, &bExist);
  zTraceP("shmget memory: %p\n", m1);

  int size = zShmSize(id1);
  zTraceP("shmctl size: %d\n", size);

  strcpy((char*)m1, "hello...");

  zTraceP("-----------------2-------------------\n");
  int id2 = 0;
  void* m2 = zShmGet(key, 0, &id2, &bExist);

  size = zShmSize(id1);
  zTraceP("shmctl size: %d\n", size);

  zTraceP("::%s\n", (char*)m2);

  zTraceP("-----------------3-------------------\n");
  int id3 = 0;
  void* m3 = zShmGet(key, 2000, &id3, &bExist);
  
  size = zShmSize(id1);
  zTraceP("shmctl size: %d\n", size);

  zTraceP("::%s\n", (char*)m3);

  zTraceP("-----------------at&dt-------------------\n");
  void* m4 = (void*)0x5000000;
  int ret = zShmAt(id1, m4);
  zTraceP("shmat return %d\n", ret);

  zTraceP("::%s\n", (char*)m4);
  
  ret = zShmDt(id1);
  //zTraceP("shmdt return %d\n", ret);  

  zTraceP("-----------------distroy-------------------\n");
  zShmDistory(id1);

  return 0;
}

#endif /*SHM_SELFTEST*/
