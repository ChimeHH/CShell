/*----------------------------------------------------------
File Name  : xxx.h
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#ifndef __Z_FASTQUE_H__
#define __Z_FASTQUE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Z_FAST_QUEUE_STAT_TYPE
{
  int high_watermark;
  int enq_succ;
  int enq_drop;
  int enq_err;
  int deq_succ;
  int deq_drop;  
  int deq_err;
} FastQueStat_t;

typedef struct Z_FAST_QUEUE_TYPE
{
  const char *name;
  FastQueStat_t stat;
  
  sem_t sem;
  
  int maxSize;
  
  int rear;
  int head;  
  int maxEnding;
  
  byte_t data[1];
} FastQue_t;

#define FASTQ_SEM_INIT(fastQ) sem_init(&(((FastQue_t *)fastQ)->sem), 0, 1);
#define FASTQ_SEM_WAIT(fastQ) sem_wait(&(((FastQue_t *)fastQ)->sem))
#define FASTQ_SEM_POST(fastQ) sem_post(&(((FastQue_t *)fastQ)->sem))
#define FASTQ_SEM_DEL(fastQ)  do{}while(0)


#ifdef __cplusplus
}
#endif

#endif /*__Z_FASTQUE_H__*/






