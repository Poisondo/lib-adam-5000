/*********************************************************************************************
Project :
Version :
Date    : 10.03.2011
Author  : Шиенков Д.И.
Company :
Comments:
**********************************************************************************************/

/*! \file typedefs.h

    Аббревиатура модуля (файла) "typedefs" - Пользовательские типы данных для проекта TM SEM.

    Для того чтобы в проекте не объявлять переменные длинными типами данных
    (например unsigned long, unsigned char и т.п.), типа для экономии места в окне редактора и т.п.,
    введены новые типы данных, которые не загромождают код.

    Хотя, можно обойтись и без этого файла.

*/

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef char s8;
typedef short s16;
typedef long s32;

#ifdef __cplusplus
}
#endif

#endif
