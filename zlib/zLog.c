/*----------------------------------------------------------
File Name  : xxx.c
Description: 
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include "zType_Def.h"

#include "zTrace.h"
#include "zTraceApi.h"


int _g_zTraceMemSaved = 0;
int _g_zTraceMemSizeMax = 0;
char* _g_zTraceMemNextPtr = 0;
char* _g_zTraceMemBuffPtr = 0;

int _zTrace2MemoryCbk(const char *pData, int nData)
{
  int right;

  if(nData > 256) /*Users to define*/
  {
    nData = 256;
  }

  if(!pData)
  {
    return -1;
  }

  _g_zTraceMemSaved += nData;

  right = _g_zTraceMemSizeMax-(_g_zTraceMemNextPtr-_g_zTraceMemBuffPtr);
  if(nData <= right)
  {
    memcpy(_g_zTraceMemNextPtr, pData, nData);

    _g_zTraceMemNextPtr += nData;
  }
  else
  {
    int left  = nData - right;

    memcpy(_g_zTraceMemNextPtr, pData, right);

    memcpy(_g_zTraceMemBuffPtr, pData, left);

    _g_zTraceMemNextPtr = _g_zTraceMemBuffPtr + left;
  }

  return nData;
}


int zTraceMemoryReset()
{
  _g_zTraceMemNextPtr = _g_zTraceMemBuffPtr;

  _g_zTraceMemSaved = 0;

  return 0;
}

int zTraceMemoryInit(int size)
{
  char *pLast = 0;

  if(size < 1024)
    size = 1024;

  if(size != _g_zTraceMemSizeMax)
  {
    pLast = _g_zTraceMemBuffPtr;

    _g_zTraceMemBuffPtr = (char*)malloc(size+1);
    if(!_g_zTraceMemBuffPtr)
    {
      zTraceError( "Init trace memory failed, required size: %d\n", size);

      return -1;
    }

    if(pLast) free(pLast);
  }

  _g_zTraceMemSizeMax = size;

  zTraceMemoryReset();

  zTraceDebug( "Trace2Mem alloced: %p  %d\n", _g_zTraceMemBuffPtr, _g_zTraceMemSizeMax);

  z_Log2MemoryCbkSet(_zTrace2MemoryCbk);

  return 0;
}


int _zTraceMemoryShow()
{
  char *p;

  zTraceP("\n");

  if(_g_zTraceMemSaved > _g_zTraceMemSizeMax)
  {
    for(p=_g_zTraceMemNextPtr; p<_g_zTraceMemBuffPtr+_g_zTraceMemSizeMax; p++)
    {
      if(0 != (*p))
      {
        zTraceP("%c", *p);
      }
    }
  }

  for(p=_g_zTraceMemBuffPtr; p<_g_zTraceMemNextPtr; p++)
  {
    if(0 != (*p))
    {
      zTraceP("%c", *p);
    }
  }

  zTraceP("\n\n");

  return 0;
}


int zTraceMemoryShow(int detail)
{
  zTraceP("Trace Memory: %p Current Offset: %d\n",
                  _g_zTraceMemBuffPtr, (int)(_g_zTraceMemNextPtr-_g_zTraceMemBuffPtr));
  zTraceP("      Total:  %d     Saved:          %d\n", _g_zTraceMemSizeMax, _g_zTraceMemSaved);

  if(detail && _g_zTraceMemSaved)
  {
#if 0  
    /*zMassShow(_zTraceMemoryShow, 0);*/
#else    
    _zTraceMemoryShow();
#endif
  }

  return 0;
}


int zTraceMemoryShowStop()
{
#if 0
  /*zMassShowStop(_zTraceMemoryShow);*/
#endif
  
  return 0;
}



