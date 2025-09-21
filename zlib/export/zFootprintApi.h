/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/

#ifndef FOOTPRINT_API_H
#define FOOTPRINT_API_H

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif


IMPORT int zEosPeg(const char* errstr, dword_t param0, dword_t param1, const char* function, int line);

IMPORT int zEosShow(const char* errstr);

/*overrides the per nodal EOS_SET*/
#undef  EOS_SET
#define EOS_SET(eos) zEosPeg(_STR(eos), 0, 0, __FUNCTION__, __LINE__)

#undef  EOS_SET_WP
#define EOS_SET_WP(eos, param0, param1) zEosPeg(_STR(eos), (dword_t)param0, (dword_t)param1, __FUNCTION__, __LINE__)



IMPORT void* zFootprintInit(int maxFootprints, int bReentrant);

IMPORT int zFootprintShow(void* serviceCore, int count);

IMPORT int zFootprintAdd(void* serviceCore, const char* errstr, const char* function, int line);
#define FOOTPRINT(serviceCore, errstr) zFootprintAdd(serviceCore, errstr, __FUNCTION__, __LINE__)


#ifdef __cplusplus
}
#endif


#endif /*FOOTPRINT_API_H*/
