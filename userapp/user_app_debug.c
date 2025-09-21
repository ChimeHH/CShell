/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com
Date       : 
------------------------------------------------------------*/
#include "zType_Def.h"

int DemoVar1 = 1;
char DemoVar2 = 'A';
long DemoVar3 = 0x1020;

int DemoCall1(int a)
{
  printf("input a=%d\n", a);
  return 0;
}

int DemoCall2(char *name, int a)
{
  printf("input %s=%d\n", name, a);
  return 0;
}

int DemoCall3(int a, char b, long c)
{
  printf("input int %d, char %c, long %lx\n", a, b, c);
  return 0;
}
