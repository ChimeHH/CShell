/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/  
#define _GNU_SOURCE
#include "string.h"

#include "zType_Def.h"

#include "zTraceApi.h"

#include "zAutoCodeApi.h"

#ifdef ENUM_PRIMARY_FUNCTIONS
char *zEnum2Name(dword_t e, zEnum2NameMap_t arr[], int nSize)
{
  int i;

  for(i=0; i<nSize; i++)
  {
    if(arr[i].e == e) return arr[i].name;
  }
  
  return 0;
}

int zName2Enum(char* name, zEnum2NameMap_t arr[], int nSize, int bAppro)
{
  int i;

  for(i=0; i<nSize; i++)
  {
    if(bAppro)
    {
      if(strcasestr(arr[i].name, name)) return arr[i].e;
    }
    else
    {
      if(!strcmp(arr[i].name, name)) return arr[i].e;
    }
  }
  
  return -1;
}

int zEnumMapShow(zEnum2NameMap_t arr[], int nSize)
{
  int i;
  for(i=0; i<nSize; i++)
  {
    zTraceP("%3d %d-> %s \n", i, arr[i].e, arr[i].name);
  }
  
  return 0;
}
#endif /*ENUM_PRIMARY_FUNCTIONS*/


#ifdef XPRINT_PRIMARY_FUNCTIONS

int xprint_device_null(const char* fmt, ...)
{
  return 0; /*do nothing; so the xprint api returns size without printing anything*/
}
int xprint_Point(int (*fnPrint)(), char* prefix, char* _sdata, void* data)
{
  if(_sdata)  
    fnPrint("%s.%s=%p \n", prefix, _sdata, data);
  else
    fnPrint("%p", data);

  return sizeof(void*);
}
int xprint_Float(int (*fnPrint)(), char* prefix, char* _sdata, float data)
{
  if(_sdata)  
    fnPrint("%s.%s=%f \n", prefix, _sdata, data);
  else
    fnPrint("%f", data);

  return sizeof(float); 
}
int xprint_Long(int (*fnPrint)(), char* prefix, char* _sdata, long data)
{
  if(_sdata)  
    fnPrint("%s.%s=%08x %08x\n", prefix, _sdata, data/0xFFFFFFFF, data&0xFFFFFFFF);
  else
    fnPrint("%08x %08x", data/0xFFFFFFFF, data&0xFFFFFFFF);

  return sizeof(long);
}
int xprint_Int(int (*fnPrint)(), char* prefix, char* _sdata, int data)
{
  if(_sdata)  
    fnPrint("%s.%s=%d \n", prefix, _sdata, data);  
  else
    fnPrint("%d", data);

  return sizeof(int);
}
int xprint_Lword(int (*fnPrint)(), char* prefix, char* _sdata, lword_t data)
{
  if(_sdata)  
    fnPrint("%s.%s=%lx \n", prefix, _sdata, data);
  else
    fnPrint("%lx", data);

  return sizeof(long);
}
int xprint_Dword(int (*fnPrint)(), char* prefix, char* _sdata, dword_t data)
{
  if(_sdata)  
    fnPrint("%s.%s=%08x \n", prefix, _sdata, data);
  else
    fnPrint("%08x", data);

  return sizeof(dword_t);
}
int xprint_Word(int (*fnPrint)(), char* prefix, char* _sdata, word_t data)
{
  if(_sdata)  
    fnPrint("%s.%s=%04x \n", prefix, _sdata, data);
  else
    fnPrint("%04x", data);

  return sizeof(word_t);
}
int xprint_Byte(int (*fnPrint)(), char* prefix, char* _sdata, byte_t data)
{
  if(_sdata)  
    fnPrint("%s.%s=%02x \n", prefix, _sdata, data);
  else
    fnPrint("%02x", data);

  return sizeof(byte_t);
}
int xprint_Char(int (*fnPrint)(), char* prefix, char* _sdata, char data)
{
  if(_sdata)  
    fnPrint("%s.%s='%c' \n", prefix, _sdata, data);
  else
    fnPrint("%c", data);

  return sizeof(char);
}
int xprint_String(int (*fnPrint)(), char* prefix, char* _sdata, char* data)
{
  if(_sdata)  
    fnPrint("%s.%s=\"%s\" \n", prefix, _sdata, data);
  else
    fnPrint("%s", data);

  return strlen(data);
}

#endif /*XPRINT_PRIMARY_FUNCTIONS*/
