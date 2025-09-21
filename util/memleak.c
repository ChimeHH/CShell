/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/  
#include "zType_Def.h"

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#ifdef realloc
#undef realloc
#endif


/** copy below lines to a common header file

#define malloc(size) tml_malloc(size, __FUNCTION__, __LINE__)
#define realloc(mem, size) tml_malloc(mem, size, __FUNCTION__, __LINE__)
#define free(mem)      tml_free(mem)
IMPORT void* tml_malloc(int size, const char* function, int line);
IMPORT void  tml_free(void* mem);
IMPORT void* tml_realloc(void* mem, int size, const char* function, int line);

** copy ends*/

//#include "zTraceApi.h"   //replace zlib with stdlib, to make the tool be lighter
//#include "zSalOS.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "memleak.h"


static MemLeak_t g_memleaks[128] = { {0,}, };



static int tml_insert(void* mem, int size, const char* function, int line)
{
  int i;
  for(i=0; i<TBL_SIZE(g_memleaks); i++)
  {
    if(g_memleaks[i].mem) continue;
    g_memleaks[i].mem = mem;
    g_memleaks[i].size = size;
    g_memleaks[i].function = function;
    g_memleaks[i].line = line;
    g_memleaks[i].tAlloc = time(0); //zTime(0);

    return 1;
  }

  return 0;
}

static int tml_remove(void* mem)
{
  int i;
  for(i=0; i<TBL_SIZE(g_memleaks); i++)
  {
    if(g_memleaks[i].mem != mem) continue;
    g_memleaks[i].mem = 0;

    return 1;
  }
  
  return 0;
}

void* tml_realloc(void* mem, int size, const char* function, int line)
{
  if(mem) //note, a null memory must cause exception and crash
  {
    tml_remove(mem);
  }

  mem = realloc(mem, size);

  if(mem)
  {
    tml_insert(mem, size, function, line);
  }

  return mem;
}

void* tml_malloc(int size, const char* function, int line)
{
  void *mem = malloc(size);
  
  if(mem) tml_insert(mem, size, function, line);
  
  return mem;
}

void  tml_free(void* mem)
{
  if(mem) //note, a null memory must cause exception and crash
  {
    tml_remove(mem);
  }

  free(mem);

  return;
}

int tml_cleanall()
{
  memset(&g_memleaks[0], 0, sizeof(g_memleaks));
  return 0;
}

int tml_show(int ref_seconds) //only those memory allocated before ref_seconds are leak candidates
{
  int now = time(0);
  
  int i;
  for(i=0; i<TBL_SIZE(g_memleaks); i++)
  {
    if(!g_memleaks[i].mem) continue;
    if(now - g_memleaks[i].tAlloc <= ref_seconds) continue;

    printf("mem: %p size: %6d  @%s:%d  tAlloc: -%d\n", g_memleaks[i].mem, g_memleaks[i].size, g_memleaks[i].function, g_memleaks[i].line, now-g_memleaks[i].tAlloc);
  }
  return 1;
}