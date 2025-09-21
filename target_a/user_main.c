/*----------------------------------------------------------
File Name  : xxx.c/.h
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"

#include "cshell.h"

int main(int argc, char**argv)
{
  loadsymbols();

  zTraceServiceInit();

  while(1)
  {
    static char input[256];

    printf("->");

    fgets(input, 255, stdin);

    input[255] = 0;

    shell_call(input);    
  }
  
  return 0;
}

#include <stdlib.h>

void* testMalloc(int size)
{
  return malloc(size);
}
void testFree(void* mem)
{
  free(mem);
  return;
}

