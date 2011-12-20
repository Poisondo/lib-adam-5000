/*********************************************************************************************
Project :
Version :
Date    : 10.03.2011
Author  : Denis Shienkov
Company :
Comments:
**********************************************************************************************/

/*! \file platformdefs.h
 *
 * In this file you must add your compiler type and determine the basic types of data.
 *
 */

#ifndef PLATFORMDEFS_H
#define PLATFORMDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__WATCOMC__) 
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef char s8;
typedef short s16;
typedef long s32;
#else
#  error "Your compiler is not supported. Please add it to platformdefs.h"
#endif

#ifdef __cplusplus
}
#endif

#endif
