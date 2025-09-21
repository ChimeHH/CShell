/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/

#ifndef AUTO_CODE_API_H
#define AUTO_CODE_API_H

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif


#define ENUM_PRIMARY_FUNCTIONS
#define XPRINT_PRIMARY_FUNCTIONS

/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
#ifdef ENUM_PRIMARY_FUNCTIONS
extern char *zEnum2Name(dword_t e, zEnum2NameMap_t arr[], int nSize);
extern int zName2Enum(char* name, zEnum2NameMap_t arr[], int nSize, int bAppro);
extern int zEnumMapShow(zEnum2NameMap_t arr[], int nSize);
#endif /*ENUM_PRIMARY_FUNCTIONS*/

/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
#ifdef XPRINT_PRIMARY_FUNCTIONS
extern int xprint_device_null(const char* fmt, ...);
extern int xprint_Point(int (*fnPrint)(), char* prefix, char* _sdata, void* data);
extern int xprint_Float(int (*fnPrint)(), char* prefix, char* _sdata, float data);
extern int xprint_Long(int (*fnPrint)(), char* prefix, char* _sdata, long data);
extern int xprint_Int(int (*fnPrint)(), char* prefix, char* _sdata, int data);
extern int xprint_Lword(int (*fnPrint)(), char* prefix, char* _sdata, lword_t data);
extern int xprint_Dword(int (*fnPrint)(), char* prefix, char* _sdata, dword_t data);
extern int xprint_Word(int (*fnPrint)(), char* prefix, char* _sdata, word_t data);
extern int xprint_Byte(int (*fnPrint)(), char* prefix, char* _sdata, byte_t data);
extern int xprint_Char(int (*fnPrint)(), char* prefix, char* _sdata, char data);
extern int xprint_String(int (*fnPrint)(), char* prefix, char* _sdata, char* data);
#endif /*XPRINT_PRIMARY_FUNCTIONS*/


extern int testFunction();

#ifdef __cplusplus
}
#endif


#endif /*AUTO_CODE_API_H*/
