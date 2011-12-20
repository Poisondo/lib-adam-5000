/*********************************************************************************************
Project :
Version :
Date    : 30.03.2011
Author  : Denis Shienkov 
Company :
Comments: 
License : New BSD
**********************************************************************************************/

/*! \file mio.h
 *
 * Abbreviation of the module (file) "mio" - Module Input Output.
 *
 *
 */

#ifndef MIO_H
#define MIO_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

void get_di_val(int slot, int *val);
void set_do_bit(int slot, int bit, int val);

#ifdef __cplusplus
}
#endif
#endif // MIO_H
