/*********************************************************************************************
Project :
Version :
Date    : 24.03.2011
Author  : Denis Shienkov 
Company :
Comments:
License : New BSD
**********************************************************************************************/

/*! \file uartbus.h
 *
 * Abbreviation of the module (file) "uartbus" - UART Bus.
 *
 * The file contains the declaration (interface) functions to access the low-speed 
 * analog I/O module (5017+ / 5018+) through their internal UART bus.
 *
 */

#ifndef UARTBUS_H
#define UARTBUS_H

#ifdef __cplusplus
extern "C" {
#endif

void uart_bus_init(int base);
void uart_bus_reset(int base);
int uart_bus_tx_empty(int base);
void uart_bus_tx(int base, const char *c);
int uart_bus_rx_ready(int base);
void uart_bus_rx(int base, char *c);

#ifdef __cplusplus
}
#endif
#endif

