/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/



#ifndef __Z_GLOBAL_H__
#define __Z_GLOBAL_H__

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif


enum ServiceCoreType
{
  eServiceNULL = 0,
  eServiceTIMER,
  eServiceOM,
  eServiceMSG,  
  eServiceFSM,
  eServiceQUE,
  
  eServiceDATAPOOL,
};


int zServiceCoreAdd(void* serviceCore, int serviceType, const char* name, const char* function, int line);
int zServiceCoreDel(void* serviceCore);

int zServiceCoreShow();
void* zServiceCoreGet(int serviceType, const char* name);


#ifdef __cplusplus
}
#endif

#endif /*__Z_GLOBAL_H__*/






