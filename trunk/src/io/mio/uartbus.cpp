/*********************************************************************************************
Project :
Version :
Date    : 24.03.2011
Author  : Denis Shienkov 
Company :
Comments:
License : New BSD
**********************************************************************************************/

/*! \file uartbus.cpp
 *
 * Abbreviation of the module (file) "uartbus" - UART Bus.
 *
 */

#include <conio.h>
#include "uartbus.h"





/*! 
 * Initializes the UART bus module with base address \a base.
 * \param base The base address of the module slot.
 * See base module address in mio.cpp.
 */
void uart_bus_init(int base)
{
    base += 3; //base + 3
    int base4 = base + 1;//base + 4
    outp(base4, 0x03);
    outp(base, 0x80);
    outp(base4, 0x00);
    outp(base, 0x02);
    outp(base4, 0x01);
    outp(base, 0x00);
    outp(base4, 0x03);
    outp(base, 0x03);
    outp(base4, 0x01);
    outp(base, 0x00);
    outp(base4, 0x02);
    outp(base, 0x07);
}

/*! 
 * Resets or clears the UART bus module with base address \a base.
 * \param base The base address of the module slot.
 * See base module address in mio.cpp.
 */
void uart_bus_reset(int base)
{
    outp((base += 6), 0x01);
    for (int i = 0; i < 1000; ++i);//delay
    outp(base, 0x00);
}

/*! 
 * Checks the possibility of sending a UART bus module with base address 
 * \a base data, ie checks whether empty TX FIFO buffer UART.
 * \param base The base address of the module slot.
 * See base module address in mio.cpp.
 * \return 0 if FIFO is not empty.
 */
int uart_bus_tx_empty(int base)
{
    outp(base + 4, 0x05);
    return ((inpw(base + 3) & 0x60) == 0x60);
}

/*! 
 * Sends the UART bus module with base address \a base one character \a c.
 * \note Before sending a character must always make sure (wait) that the
 * transmitting UART TX FIFO buffer is empty uart_bus_tx_empty(), and 
 * only then send the character!
 * \param base The base address of the module slot.
 * See base module address in mio.cpp.
 * \param c A pointer to a particular character in an external buffer
 * in which data are available for transmission.
 */
void uart_bus_tx(int base, const char *c)
{
    outp(base + 4, 0x00);
    outp(base + 3, *c);
}

/*! 
 * Verifies readiness for reading data from the UART bus module with base address \a base,
 * ie checks whether there is data to read from the RX FIFO buffer UART.
 * \param base The base address of the module slot.
 * See base module address in mio.cpp.
 * \return 0 if no data to read.
*/
int uart_bus_rx_ready(int base)
{
    outp(base + 4, 0x05);
    return ((inpw(base + 3) & 0x01) == 0x01);// == или != ???
}

/*! 
 * Reads from UART bus module with base address \a base one character \a c.
 * \note Before reading should always make sure (wait) that the receiving
 * UART RX FIFO buffer has data uart_bus_rx_ready(), and only then read
 * the character!
 * \param base The base address of the module slot.
 * See base module address in mio.cpp.
 * \param c A pointer to a particular character in a sort of external
 * buffer into which data is read at the reception.
 */
void uart_bus_rx(int base, char *c)
{
    outp(base + 4, 0x00);
    *c = (char)inpw(base + 3);
}


