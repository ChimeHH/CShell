
/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"
#include "zSalOS.h"
#undef printf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#define Z_SHELL_TRACE_LOCK_ON() do{}while(0)
#define Z_SHELL_TRACE_LOCK_OFF() do{}while(0)
#define Z_TASK_TRACE_LOCK_ON() do{}while(0)
#define Z_TASK_TRACE_LOCK_OFF() do{}while(0)

extern int z_Log2ConsoleDefault(const char *pData, int nLen);

int (*g_zLog2ConsoleOutputCbk)(const char*, int) = z_Log2ConsoleDefault;
int (*g_zLog2ZcnOutputCbk)(const char*, int) = 0;
int (*g_zLog2FileOutputCbk)(const char*, int) = 0;
int (*g_zLog2MemoryOutputCbk)(const char*, int) = 0;

int z_Log2ConsoleCbkSet(int (*fnPrint)(const char *, int))
{
  g_zLog2ConsoleOutputCbk = fnPrint;

  return 0;
}

int z_Log2ZcnCbkSet(int (*fnPrint)(const char *, int))
{
  g_zLog2ZcnOutputCbk = fnPrint;

  return 0;
}

int z_Log2FileCbkSet(int (*fnPrint)(const char *, int))
{
  g_zLog2FileOutputCbk = fnPrint;

  return 0;
}

int z_Log2MemoryCbkSet(int (*fnPrint)(const char *, int))
{
  g_zLog2MemoryOutputCbk = fnPrint;

  return 0;
}

int z_ShellLog(const char *fmt, ...)
{
  int count = 0;
  va_list arg;

  static char str[2048];

  char *pCur = str;
  
  Z_SHELL_TRACE_LOCK_ON();
  
  va_start (arg, fmt);
  pCur += vsprintf(pCur ,fmt, arg);
  va_end (arg);

  *pCur = 0;
  
  count = pCur - &str[0];

  /* Disable console redirection.
  if(g_zLog2ConsoleOutputCbk)
  {
    g_zLog2ConsoleOutputCbk(str, count);
  }
  */
  printf("%s", str);
  
  if(g_zLog2ZcnOutputCbk)
  {
    g_zLog2ZcnOutputCbk(str, count);
  }

  if(g_zLog2FileOutputCbk)
  {
    g_zLog2FileOutputCbk(str, count);
  }

  if(g_zLog2MemoryOutputCbk)
  {
    g_zLog2MemoryOutputCbk(str, count);
  }

  Z_SHELL_TRACE_LOCK_OFF();
  
  return 0;
}

int z_IntLog  (const char *fmt, ...)
{
  //int count = 0;
  va_list arg;

  static char str[2048];

  char *pCur = str;
    
  va_start (arg, fmt);
  pCur += vsprintf(pCur ,fmt, arg);
  va_end (arg);

  *pCur = 0;

  //count = pCur - &str[0] + 1;   
  //z_MessageQSend(z_IntLogQueue(), str, count);

  return 0;
}


int z_TaskLog (const char *fmt, ...)
{
  int count = 0;

  static char str[2048];
  va_list arg;

  char *pCur = str;
  
  Z_TASK_TRACE_LOCK_ON();
  
  va_start (arg, fmt);
  pCur += vsprintf(pCur ,fmt, arg);
  va_end (arg);

  *pCur = 0;
  
  count = pCur - &str[0];

  if(g_zLog2ConsoleOutputCbk)
  {
    g_zLog2ConsoleOutputCbk(str, count);
  }
  
  if(g_zLog2ZcnOutputCbk)
  {
    g_zLog2ZcnOutputCbk(str, count);
  }

  if(g_zLog2FileOutputCbk)
  {
    g_zLog2FileOutputCbk(str, count);
  }

  if(g_zLog2MemoryOutputCbk)
  {
    g_zLog2MemoryOutputCbk(str, count);
  }

  Z_TASK_TRACE_LOCK_OFF();
  
  return 0;
}


int z_ShellLogHex(const byte_t *pData, int nLen)
{
  int i;

  static char str[2048];
  
  Z_SHELL_TRACE_LOCK_ON();

  int count = 0;
  int nLen2 = 0;
  
  print_more:

  count = 0;
  nLen2 = MIN(64, nLen);
  
  for(i=0; i<nLen2; i++)
  {
    count += sprintf(&str[count], "%02x", pData[i]);

    if(i < nLen2)
    {
      count += sprintf(&str[count], " ");
      if((i & 0x03) == 0x03) count += sprintf(&str[count], " ");
      //if((i & 0x0F) == 0x0F) count += sprintf(&str[count], "  ");      
      if((i & 0x1F) == 0x1F) count += sprintf(&str[count], "\n");
    }
  }

  str[count] = 0;
  
  //Disable console redirection
  //if(g_zLog2ConsoleOutputCbk) g_zLog2ConsoleOutputCbk(str, count);  
  printf("%s", str);
  if(g_zLog2ZcnOutputCbk) g_zLog2ZcnOutputCbk(str, count);
  if(g_zLog2FileOutputCbk) g_zLog2FileOutputCbk(str, count);
  if(g_zLog2MemoryOutputCbk) g_zLog2MemoryOutputCbk(str, count);

  if(nLen2 < nLen)
  {    
    nLen -= nLen2;
    pData += nLen2;
    goto print_more;
  }


  Z_SHELL_TRACE_LOCK_OFF();
  
  return 0;
}

int z_IntLogHex  (const byte_t *pData, int nLen)
{
  int count = 0;

  int i;

  static char str[2048];

  for(i=0; i<nLen; i++)
  {
    count += sprintf(&str[count], "%02x", pData[i]);

    if(i < nLen)
    {
      count += sprintf(&str[count], " ");
      if((i & 0x03) == 0x03) count += sprintf(&str[count], " ");
      //if((i & 0x0F) == 0x0F) count += sprintf(&str[count], "  ");
      if((i & 0x1F) == 0x1F) count += sprintf(&str[count], "\n");
    }
  }

  str[count] = 0;
  
#if 0  
  z_MessageQSend(z_IntLogQueue(), str, count);
#endif

  return 0;
}

int z_TaskLogHex (const byte_t *pData, int nLen)
{  
  int i;

  static char str[2048]; //

    
  Z_TASK_TRACE_LOCK_ON();

  int count = 0;
  int nLen2 = 0;
  
  print_more:

  count = 0;
  nLen2 = MIN(64, nLen);

  for(i=0; i<nLen2; i++)
  {
    count += sprintf(&str[count], "%02x", pData[i]);

    if(i < nLen2)
    {
      count += sprintf(&str[count], " ");
      if((i & 0x03) == 0x03) count += sprintf(&str[count], " ");
      if((i & 0x0F) == 0x0F) count += sprintf(&str[count], "  ");
      if((i & 0x1F) == 0x1F) count += sprintf(&str[count], "\n");
    }
  }

  str[count] = 0;

  if(g_zLog2ConsoleOutputCbk) g_zLog2ConsoleOutputCbk(str, count);  
  if(g_zLog2ZcnOutputCbk) g_zLog2ZcnOutputCbk(str, count);
  if(g_zLog2FileOutputCbk) g_zLog2FileOutputCbk(str, count);
  if(g_zLog2MemoryOutputCbk) g_zLog2MemoryOutputCbk(str, count);

  if(nLen2 < nLen)
  {    
    nLen -= nLen2;
    pData += nLen2;
    goto print_more;
  }

  Z_TASK_TRACE_LOCK_OFF();
  
  return 0;
}


int z_ShellPrint(const char *fmt, ...)
{ 
  va_list arg;
  
  va_start (arg, fmt);
  vprintf(fmt, arg);
  va_end (arg);
  
  return 0;
}

int z_ShellPrintHex(const byte_t *pData, int nLen)
{
  int i;
    
  for(i=0; i<nLen; i++)
  {
    printf("%02x", pData[i]);
    if(i < nLen)
    {
      printf(" ");
      if((i & 0x03) == 0x03) printf("  ");
      if((i & 0x1F) == 0x1F) printf("\n");
    }
  }
  
  return 0;
}

int z_Log2ConsoleDefault(const char *pData, int nLen)
{
  int i;

  for(i=0; i<nLen; i++)
  {
    if(pData[i])
    {
      putchar(pData[i]);
    }
  }

  return 0;
}

