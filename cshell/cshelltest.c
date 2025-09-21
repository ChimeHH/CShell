/*----------------------------------------------------------
File Name  : xxx.c
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int testIV(void)
{
  printf("%s\n",__FUNCTION__);
  return 0;
}
void testVV(void)
{
  printf("%s\n",__FUNCTION__);
  return;
}
int testII(int i)
{
  printf("%s %d\n",__FUNCTION__, i);
  return i;
}
int testIICS(int i, char c, char* s)
{
  printf("%s %d %c %s\n",__FUNCTION__, i, c, s);
  return i;
}





