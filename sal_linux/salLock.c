/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "zType_Def.h"
#include "zSalOS.h"

#include "zTraceApi.h"

/********************************************************************
                          MUTEX LOCK
*********************************************************************/
void *zMutexInit()
{
  pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
  if(!mutex)
  {
    zTraceAlarm("Memory downflow.\n");
    return 0;
  }
  
  pthread_mutex_unlock(mutex);

  return mutex;
}

int zMutexLock(void *mutex)
{
  if(mutex)  
  {
    return pthread_mutex_lock((pthread_mutex_t *)mutex);
  }
  return 0;
}

int zMutexTryLock(void *mutex)
{
  if(mutex)  
  {
    return pthread_mutex_trylock((pthread_mutex_t *)mutex);
  }
  return 0;
}

int zMutexUnlock(void *mutex)
{
  if(mutex)  
  {
    return pthread_mutex_unlock((pthread_mutex_t *)mutex);
  }
  return 0;
}

int zMutexDestory(void *mutex)
{
  if(mutex)  
  {    
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
    free(mutex);
  }
  return 0;
}


/********************************************************************
                               SPIN LOCK
*********************************************************************/
volatile void *zSpinInit()
{
  pthread_spinlock_t *spin = malloc(sizeof(pthread_spinlock_t));
  if(!spin)
  {
    zTraceAlarm("Memory downflow.\n");
    return 0;
  }

  pthread_spin_unlock(spin);

  return spin;
}

int zSpinLock(void *spin)
{
  if(spin)  
  {
    return pthread_spin_lock((pthread_spinlock_t *)spin);
  }
  return 0;
}

int zSpinTryLock(void *spin)
{
  if(spin)  
  {
    return pthread_spin_trylock((pthread_spinlock_t *)spin);
  }
  return 0;
}

int zSpinUnlock(void *spin)
{
  if(spin)  
  {
    return pthread_spin_unlock((pthread_spinlock_t *)spin);
  }
  return 0;
}

int zSpinDestory(void *spin)
{
  if(spin)  
  {    
    pthread_spin_unlock((pthread_spinlock_t *)spin);
    free(spin);
  }
  return 0;
}


/********************************************************************
                     READ & WRITE LOCK
*********************************************************************/

void *zRwLockInit()
{
  pthread_rwlock_t  *rw = malloc(sizeof(pthread_rwlock_t ));
  if(!rw)
  {
    zTraceAlarm("Memory downflow.\n");
    return 0;
  }

  pthread_rwlock_init(rw, 0);

  return rw;
}

int zRwLockRdLock(void *rw)
{
  if(rw)  
  {
    return pthread_rwlock_rdlock((pthread_rwlock_t  *)rw);
  }
  return 0;
}
int zRwLockWrLock(void *rw)
{
  if(rw)  
  {
    return pthread_rwlock_wrlock((pthread_rwlock_t  *)rw);
  }
  return 0;
}

int zRwLockTryRdLock(void *rw)
{
  if(rw)  
  {
    return pthread_rwlock_tryrdlock((pthread_rwlock_t  *)rw);
  }
  return 0;
}
int zRwLockTryWrLock(void *rw)
{
  if(rw)  
  {
    return pthread_rwlock_trywrlock((pthread_rwlock_t  *)rw);
  }
  return 0;
}

int zRwLockTimedRdLock(void *rw, int sec, int nsec)
{
  if(rw)  
  {
    struct timespec ts;
    memset(&ts, 0, sizeof(ts)); //clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += sec;
    ts.tv_nsec += nsec;    
    return pthread_rwlock_timedrdlock((pthread_rwlock_t  *)rw, &ts);
  }
  return 0;
}
int zRwLockTimedWrLock(void *rw, int sec, int nsec)
{
  if(rw)  
  {    
    struct timespec ts;
    memset(&ts, 0, sizeof(ts)); //clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += sec;
    ts.tv_nsec += nsec;    
    return pthread_rwlock_timedwrlock((pthread_rwlock_t  *)rw, &ts);
  }
  return 0;
}
int zRwLockUnlock(void *rw)
{
  if(rw)  
  {
    return pthread_rwlock_unlock((pthread_rwlock_t  *)rw);
  }
  return 0;
}

int zRwLockDestory(void *rw)
{
  if(rw)  
  {    
    pthread_rwlock_destroy((pthread_rwlock_t  *)rw);
    free(rw);
  }
  return 0;
}


//named sem, initialized to 0 (locked)
int zSemInit(int key, int *bExist)
{
  int sem = semget(key, 1, IPC_CREAT |0777 | IPC_EXCL);
  if(sem <= 0)
  {
    if(bExist) *bExist = 1;
    sem = semget(key, 1, 0777);
    if(sem <= 0)
    {  
      zTraceAlarm("Failed to get sem of %08x: errno=%d, %s.\n", key, errno, strerror(errno));
      return 0;
    }
  }
  else
  {
    semctl(sem , 0 , SETVAL , 1);
  }

  return sem;
}

int zSemLock(int sem)
{
  if(sem <= 0) return -1;

  struct sembuf sops={0, -1, SEM_UNDO};
  
  return semop(sem, &sops, 1);
}

#if 0
int zSemTimedWait(void *sem, int sec, int nsec)
{
  if(!sem) return -1;

  struct timespec ts;
  ts.tv_sec = sec;
  ts.tv_nsec = nsec+1;
  
  return sem_timedwait(sem, &ts);
}

int zSemTryLock(void *sem)
{
  if(!sem) return -1;

  return sem_trywait(sem);
}
#endif

int zSemUnlock(int sem)
{
  if(sem <= 0) return -1;

  struct sembuf sops={0, +1, SEM_UNDO};
  
  return semop((int)sem, &sops, 1);
}

int zSemDestroy(int sem)
{
  if(sem<=0) return -1;

  return semctl(sem,0,IPC_RMID, 1);
}

int zSemStatus(int sem)
{
  return semctl(sem, 0, GETVAL, 1);
}

#define LOCK_SELFTEST

#ifdef LOCK_SELFTEST

int testSem(int flag)
{
  int iRet = 0;
  
  int key = 124;
  int bExist = 0;
  int sem = zSemInit(key, &bExist);

  zTraceP("sem: %p, existed: %d\n", sem, bExist);

  if(flag == 3)
  {    
    iRet = zSemDestroy(sem);
    zTraceP("sem distroy, iRet=%d\n", iRet);
  }
  else if(flag == 1)
  {    
    iRet = zSemLock(sem);
    zTraceP("sem locked, iRet=%d\n", iRet);
  }
  else if(flag == 2)
  {
    iRet = zSemUnlock(sem);
    zTraceP("sem unlocked, iRet=%d\n", iRet);
  }
  else
  {
    iRet = zSemStatus(sem);
    zTraceP("sem status, iRet=%d\n", iRet);
  }

  return 0;
}

#endif /*LOCK_SELFTEST*/

