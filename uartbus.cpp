/*********************************************************************************************
Project :
Version :
Date    : 24.03.2011
Author  : ������� �.�.
Company :
Comments:
**********************************************************************************************/

/*! \file uartbus.cpp

    ������������ ������ (�����) "uartbus" - UART Bus.


*/

#include <conio.h>
#include "uartbus.h"





/*! \fn void uart_bus_init(int base)
    �������������� UART ���� ������ � ������� ������� \a base.
    \param base ������� ����� ������ (�����).
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

/*! \fn void uart_bus_reset(int base)
    ���������� ��� ������� UART ���� ������ � ������� ������� \a base.
    \param base ������� ����� ������ (�����).
*/
void uart_bus_reset(int base)
{
    outp((base += 6), 0x01);
    for (int i = 0; i < 1000; ++i);//delay
    outp(base, 0x00);
}

/*! \fn int uart_bus_tx_empty(int base)
    ��������� ����������� �������� � UART ���� ������ � ������� ������� \a base
    ������, �.�. ���������, ���� �� TX FIFO ����� UART.
    \param base ������� ����� ������ (�����).
    \return 0 ����� �� ����.
*/
int uart_bus_tx_empty(int base)
{
    outp(base + 4, 0x05);
    return ((inpw(base + 3) & 0x60) == 0x60);
}

/*! \fn void uart_bus_tx(int base, const char *c)
    ���������� � UART ���� ������ � ������� ������� \a base
    ���� ������ \a �.
    \note ����� ��������� ������� ���������� ����������� ��������� (���������)
    ��� ���������� TX FIFO ����� UART ���� (uart_bus_tx_empty()), � ������ ����� ����� ���������� ������!!!
    \param base ������� ����� ������ (�����).
    \param c ��������� �� ���������� ������ � �����-�� �������
    ������ � ������� ������� ������ ��� ��������.
*/
void uart_bus_tx(int base, const char *c)
{
    outp(base + 4, 0x00);
    outp(base + 3, *c);
}

/*! \fn int uart_bus_rx_ready(int base)
    ��������� ���������� ��� ������ ������ �� UART ���� ������ � ������� ������� \a base,
    �.�. ���������, ���� �� ��� ������ ��� ������ �� RX FIFO ������ UART.
    \param base ������� ����� ������ (�����).
    \return 0 ��� ��� ������ ��� ������.
*/
int uart_bus_rx_ready(int base)
{
    outp(base + 4, 0x05);
    return ((inpw(base + 3) & 0x01) == 0x01);// == ��� != ???
}

/*! \fn void uart_bus_rx(int base, char *c)
    ������ �� UART ���� ������ � ������� ������� \a base
    ���� ������ \a �.
    \note ����� ������� ���������� ����������� ��������� (���������)
    ��� ����������� RX FIFO ����� UART ����� ������ (uart_bus_rx_ready()),
    � ������ ����� ����� ������ ������!!!
    \param base ������� ����� ������ (�����).
    \param c ��������� �� ���������� ������ � �����-�� �������
    ������ � ������� ������������ ������ ������ ��� ������.
*/
void uart_bus_rx(int base, char *c)
{
    outp(base + 4, 0x00);
    *c = (char)inpw(base + 3);
}


