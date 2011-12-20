/*********************************************************************************************
Project :
Version :
Date    : 30.03.2011
Author  : Denis Shienkov 
Company :
Comments:
License : New BSD
**********************************************************************************************/

/*! \file mio.cpp
 *
 * Abbreviation of the module (file) "mio" - Module Input Output.
 *
 * It implements an API to access the local I/O modules a PLC ADAM 5510. 
 *
 * Modules I/O PLCs are divided into high and low speed.
 * 
 * Getting data from high-speed modules made ​​directly by reading the ports of these modules
 * on the base address and relevant to their shifts.
 * 
 * Getting data from low-speed modules is through the exchange of commands to the module on
 * through the internal bus of the module UART protocol type DCON.
 *
 */

#include <dos.h>
#include <conio.h>
#include "mio.h"


//--------------------------------------------------------------------------------------------------------//
/*** Private variables ***/

/*! 
 * Base addresses of slots 0 - 7 PLC. 
 * 
 * Based on the analysis of the decompiled assembler code (using IDA)
 * libraries from Advantech is clear that each of the 8 slots for landing module has
 * its base address (for example, the slot number 0 - 0x0100 address).
 * 
 * Alternate base addresses of modules through 16 registers, ie, each module
 * theoretically has 16 registers (address, offset) input/output. For each specific
 * type of module function of these registers may be different and this designation
 * can be found analyzing the decompiled assembly code libraries from Advantech!
 */
static const int board_base[8] = { 0x0100, 0x0110, 0x0120, 0x0130, 0x0140, 0x0150, 0x0160, 0x0170 };

/*! 
 * Bitmasks of each bit in a byte, simply used to optimize the 
 * algorithms and code.
 */
static const char bit_mask[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

/*!
 * Unknown the destination.
 */
static char can_xxx_proc[8] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

/*! 
 * An array of 8 elements, which are the previous values ​​of channel discrete input
 * or output modules in slots 0 - 7:
 * - If the current values ​​of channel modules with DI, these current values ​​at once
 * stored in the array (for which this is done??? you need to think!!!)
 * - The issuance of channel management modules in DO, the desired channel values ​​are stored
 * in the array (this is the type needed to indirectly determine the current state of the channel module
 * DO, as no? possible to determine the software is able to read the registers of the module!).
 */
static int dio_val[8] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

//--------------------------------------------------------------------------------------------------------//
/*** Private functions ***/

//--------------------------------------------------------------------------------------------------------//
/*** Public variables ***/

//--------------------------------------------------------------------------------------------------------//
/*** Global functions ***/

/*!
 */
void get_di_val(int slot, int *val)
{
    // Get current value at offset +2 and +3.
    *val = (inp(board_base[slot] + 3) << 8) | inp(board_base[slot] + 2);
    
    // If the function is called first then read the registers at offset +4 and +5
    // NOTE: The purpose of these registers have not yet been elucidated!
    if (can_xxx_proc[slot]) {
    
        // The next call to not read the registers at offsets +4 and +5.
        can_xxx_proc[slot] = 0;

        // Transform the values ​​of registers at offsets +4 and +5.
        {
            dio_val[slot] = ((~(inp(board_base[slot] + 5))) << 8) | (~(inp(board_base[slot] + 4)));
            int old_val = 0;
            for (int i = 0; i < 8; ++i) {
                //lo
                if ((bit_mask[i] & 0x00FF) & dio_val[slot]) { old_val += bit_mask[7 - i]; }
                //hi
                if ((bit_mask[i] << 8) & dio_val[slot]) { old_val += bit_mask[7 - i] << 8; }
            }
            dio_val[slot] = old_val;
        }
    }
    *val ^= dio_val[slot];
}

/*! 
 */
void set_do_bit(int slot, int bit, int val)
{
    // One bit set or reset.
    if ((8 > slot) && (16 > bit)) {
        if (val) { dio_val[slot] |= (1 << bit); }
        else { dio_val[slot] &= ~(1 << bit); }
        outp(board_base[slot] + 2, dio_val[slot]); //lo
        outp(board_base[slot] + 3, dio_val[slot] >> 8);//hi
    }
}


