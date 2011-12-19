/*********************************************************************************************
Project :
Version :
Date    : 24.03.2011
Author  : Шиенков Д.И.
Company :
Comments:
**********************************************************************************************/

/*! \file uartbus.cpp

    Аббревиатура модуля (файла) "uartbus" - UART Bus.


*/

#include <conio.h>
#include "uartbus.h"





/*! \fn void uart_bus_init(int base)
    Инициализирует UART шину модуля с базовым адресом \a base.
    \param base Базовый адрес модуля (слота).
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
    Сбрасывает или очищает UART шину модуля с базовым адресом \a base.
    \param base Базовый адрес модуля (слота).
*/
void uart_bus_reset(int base)
{
    outp((base += 6), 0x01);
    for (int i = 0; i < 1000; ++i);//delay
    outp(base, 0x00);
}

/*! \fn int uart_bus_tx_empty(int base)
    Проверяет возможность отправки в UART шину модуля с базовым адресом \a base
    данных, т.е. проверяет, пуст ли TX FIFO буфер UART.
    \param base Базовый адрес модуля (слота).
    \return 0 Буфер не пуст.
*/
int uart_bus_tx_empty(int base)
{
    outp(base + 4, 0x05);
    return ((inpw(base + 3) & 0x60) == 0x60);
}

/*! \fn void uart_bus_tx(int base, const char *c)
    Отправляет в UART шину модуля с базовым адресом \a base
    один символ \a с.
    \note Перед отправкой символа необходимо ОБЯЗАТЕЛЬНО убедиться (дождаться)
    что передающий TX FIFO буфер UART пуст (uart_bus_tx_empty()), и только после этого отправлять символ!!!
    \param base Базовый адрес модуля (слота).
    \param c Указатель на конкретный символ в каком-то внешнем
    буфере в котором имеются данные для передачи.
*/
void uart_bus_tx(int base, const char *c)
{
    outp(base + 4, 0x00);
    outp(base + 3, *c);
}

/*! \fn int uart_bus_rx_ready(int base)
    Проверяет готовность для чтения данных из UART шины модуля с базовым адресом \a base,
    т.е. проверяет, есть ли уже данные для чтения из RX FIFO буфера UART.
    \param base Базовый адрес модуля (слота).
    \return 0 Нет еще данных для чтения.
*/
int uart_bus_rx_ready(int base)
{
    outp(base + 4, 0x05);
    return ((inpw(base + 3) & 0x01) == 0x01);// == или != ???
}

/*! \fn void uart_bus_rx(int base, char *c)
    Читает из UART шину модуля с базовым адресом \a base
    один символ \a с.
    \note Перед чтением необходимо ОБЯЗАТЕЛЬНО убедиться (дождаться)
    что принимающий RX FIFO буфер UART имеет данные (uart_bus_rx_ready()),
    и только после этого читать символ!!!
    \param base Базовый адрес модуля (слота).
    \param c Указатель на конкретный символ в каком-то внешнем
    буфере в который производится чтение данных при приеме.
*/
void uart_bus_rx(int base, char *c)
{
    outp(base + 4, 0x00);
    *c = (char)inpw(base + 3);
}


