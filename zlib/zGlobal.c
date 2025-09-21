/*----------------------------------------------------------
File Name  : xxx.c
Description:
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>

#include "zTraceApi.h"
#include "zGlobal.h"

#include "zAutoCodeApi.h"
zEnum2NameMap_t EnumServiceCoreType_map_t[]=
{
  {eServiceNULL, "eServiceNULL"},
  {eServiceTIMER, "eServiceTIMER"},
  {eServiceOM, "eServiceOM"},
  {eServiceMSG, "eServiceMSG"},  
  {eServiceFSM, "eServiceFSM"},
  {eServiceQUE, "eServiceQUE"},
  
  {eServiceDATAPOOL, "eServiceDATAPOOL"},
}; /*EnumServiceCoreType_map_t*/

const char* EnumServiceCoreType2Name(dword_t e)
{
  char* pName = zEnum2Name(e, EnumServiceCoreType_map_t, TBL_SIZE(EnumServiceCoreType_map_t));
  
  if(pName) return pName;
  return "*NA*";
}

dword_t EnumServiceCoreType2Value(char* name, int bAppro)
{
  return zName2Enum(name, EnumServiceCoreType_map_t, TBL_SIZE(EnumServiceCoreType_map_t), bAppro);
};

int EnumServiceCoreTypeMapShow()
{
  return zEnumMapShow(EnumServiceCoreType_map_t, TBL_SIZE(EnumServiceCoreType_map_t));
};


typedef struct SERVICE_CORE_BLOCK_TYPE
{
  void* serviceCore;
  int   serviceType;
  const char* name;
  const char* function;
  int   line;
} ServiceCoreBlock_t;

static ServiceCoreBlock_t sServiceCoreList[64];

int zServiceCoreAdd(void* serviceCore, int serviceType, const char* name, const char* function, int line)
{
  if(!serviceCore)
  {
    zTraceError("invalid input null servicecore @%s:%d.\n", function, line);
    return -1;
  }

  int max = TBL_SIZE(sServiceCoreList);
  int i;
  for(i=0; i<max; i++)
  {
    ServiceCoreBlock_t *pBlk = &sServiceCoreList[i];
    if(pBlk->serviceCore) continue;
    pBlk->serviceCore = serviceCore;
    pBlk->serviceType = serviceType;
    pBlk->name = name;
    pBlk->function = function;
    pBlk->line = line;

    return 0;
  }
    
  if(i >= max)
  {
    zTraceError("servicecore overflow @%s:%d.\n", function, line);
    return -1;
  }

  return i;
}

int zServiceCoreDel(void* serviceCore)
{
  if(!serviceCore) return -1;

  int max = TBL_SIZE(sServiceCoreList);
  int i;
  for(i=0; i<max; i++)
  {
    ServiceCoreBlock_t *pBlk = &sServiceCoreList[i];
    if(pBlk->serviceCore != serviceCore) continue;
    
    pBlk->serviceCore = 0;
    return 0;
  }

  return -1;
}


int zServiceCoreShow()
{
  int max = TBL_SIZE(sServiceCoreList);
  int i;
  for(i=0; i<max; i++)
  {
    ServiceCoreBlock_t *pBlk = &sServiceCoreList[i];
    if(!pBlk->serviceCore) continue;

    zTraceP("[%3d] %p %d %16s \"%s\" @%s:%d\n", i, pBlk->serviceCore, pBlk->serviceType, EnumServiceCoreType2Name(pBlk->serviceType), 
                pBlk->name, pBlk->function, pBlk->line);
  }
  
  return 0;
}

void* zServiceCoreGet(int serviceType, const char* name)
{
  if(!name)
  {
    zTraceError("Bad input parameters.\n");
    zTraceError("zServiceCoreGet(serviceType, name), for details refer to zServiceCoreListShow()\n");
    
    return 0;
  }

  int max = TBL_SIZE(sServiceCoreList);
  int i;
  for(i=0; i<max; i++)
  {
    ServiceCoreBlock_t *pBlk = &sServiceCoreList[i];
    if(!pBlk->serviceCore) continue;
    if(!pBlk->name) continue;
    if(serviceType && (serviceType != pBlk->serviceType)) continue;
    
    if(strcmp(pBlk->name, name)) continue;
    
    return pBlk->serviceCore;
  }

  return 0;
}



