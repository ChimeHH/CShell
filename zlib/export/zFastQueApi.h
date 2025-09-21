/*----------------------------------------------------------
File Name  : xxx.h
Description: This file should be included by any products,
             no matter on vxworks or windows.
Author     : hhao020@gmail.com (bug fixing and consulting)
Date       : 2007-05-15
------------------------------------------------------------*/

#ifndef FAST_QUE_API_H
#define FAST_QUE_API_H

#include "zType_Def.h"

#ifdef __cplusplus
extern "C" {
#endif


void* zFastQueInitEx(const char* name, int maxCount, int maxSize, const char* function, int line);


int zFastEnqueEx(void *que, void *msg, int size, const char* function, int line);
int zFastDequeEx(void *que, void* msg, int size, const char* function, int line);

#define zFastQueInit(name, maxDepth, maxWidth) zFastQueInitEx(_STR(name), maxDepth, maxWidth, __FUNCTION__, __LINE__)
#define zFastEnque(que, msg, size) zFastEnqueEx(que, msg, size, __FUNCTION__, __LINE__)
#define zFastDeque(que, msg, size) zFastDequeEx(que, msg, size, __FUNCTION__, __LINE__)

int zFastQueShow(void *que);


#ifdef __cplusplus
}
#endif


#endif /*FAST_QUE_API_H*/
