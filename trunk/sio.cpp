/*********************************************************************************************
Project :
Version :
Date    : 24.03.2011
Author  : Дьяконов О.Ю., Шиенков Д.И.
Company :
Comments: Библиотека для работы с последовательными портами в контроллерах ADAM 5510.
**********************************************************************************************/

/*! \file sio.cpp

    Аббревиатура модуля (файла) "sio" - Serial Input Output.

    В этом модуле реализован интерфейс для доступа и работы с последовательными
    портами COM1 - COM4 ПЛК серии ADAM 5510.
*/

#include "sio.h"

#include <dos.h>
#include <mem.h>
#include <malloc.h>
#include <conio.h>


//--------------------------------------------------------------------------------------------------------//
/*** ЛОКАЛЬНЫЕ ПЕРЕЧИСЛЕНИЯ АДРЕСОВ И ЗНАЧЕИЙ БИТОВ РЕГИСТРОВ ***/

/*! \enum sio_offset_t
    Смещения регистров UART от базового.
    Всего в UART должно быть 12 регистров.
*/
typedef enum SIO_OFFSET {
    SIO_OFFSET_THB  = 0, /*!< Transmitter Holding BuFFer,        wo, DLAB = 0. */
    SIO_OFFSET_RB   = 0, /*!< Receiver BuFFer,                   ro, DLAB = 0. */
    SIO_OFFSET_DLLB = 0, /*!< Divisor Latch Low Byte,            rw, DLAB = 1. */
    SIO_OFFSET_IER  = 1, /*!< Interrupt Enable Register,         rw, DLAB = 0. */
    SIO_OFFSET_DLHB = 1, /*!< Divisor Latch High Byte,           rw, DLAB = 1. */
    SIO_OFFSET_IIR  = 2, /*!< Interrupt Identification Register, ro.           */
    SIO_OFFSET_FCR  = 2, /*!< FIFO Control Register, ro.         wo.           */
    SIO_OFFSET_LCR  = 3, /*!< Line Control Register,             rw.           */
    SIO_OFFSET_MCR  = 4, /*!< Modem Control Register,            rw.           */
    SIO_OFFSET_LSR  = 5, /*!< Line Status Register,              ro.           */
    SIO_OFFSET_MSR  = 6, /*!< Modem Status Register,             ro.           */
    SIO_OFFSET_SCR  = 7  /*!< Scratch Register,                  rw.           */
} sio_offset_t;

/*! \enum sio_ier_b_t
    Значения комбинаций битов регистра IER.
    Доступ: rw, при DLAB = 0.
*/
typedef enum SIO_IER_B {
    SIO_IER_ERDAI  = 0x01, /*!< Enable Received Data Available Interrupt,            > 0x00. */
    SIO_IER_ETHREI = 0x02, /*!< Enable Transmitter Holding Register Empty Interrupt, > 0x00. */
    SIO_IER_ELSI   = 0x04, /*!< Enable Receiver Line Status Interrupt,               > 0x00. */
    SIO_IER_EMSI   = 0x08, /*!< Enable Modem Status Interrupt,                       > 0x00. */
    SIO_IER_ESM    = 0x10, /*!< Enables Sleep Mode (16750),                          > 0x00. */
    SIO_IER_ELPM   = 0x20, /*!< Enables Low Power Mode (16750),                      > 0x00. */
    SIO_IER_RES1   = 0x40, /*!< Reserved.                                                    */
    SIO_IER_RES2   = 0x80  /*!< Reserved.                                                    */
} sio_ier_b_t;

/*! \enum sio_iir_b_t
    Значения комбинаций битов регистра IIR.
    Доступ: ro.
*/
typedef enum SIO_IIR_B {
    SIO_IIR_IP      = 0x01, /*!< 0 = Interrupt Pending, 1 = No Interrupt Pending.      */
    SIO_IIR_ID_MASK = 0x0E, /*!< Interrupt ID Mask.                                    */
    SIO_IIR_MSI     = 0x00, /*!< Modem Status Interrupt,                       = 0x00. */
    SIO_IIR_THREI   = 0x02, /*!< Transmitter Holding Register Empty Interrupt, = 0x02. */
    SIO_IIR_RDAI    = 0x04, /*!< Received Data Available Interrupt,            = 0x04. */
    SIO_IIR_RLSI    = 0x06, /*!< Receiver Line Status Interrupt,               = 0x06. */
    SIO_IIR_RDTO    = 0x0C  /*!< Receiver Dtata timeout,                       = 0x0C. */
} sio_iir_b_t;

/*! \enum sio_fcr_b_t
    Значения комбинаций битов регистра FCR.
    Доступ: wo.
*/
typedef enum SIO_FCR_B {
    SIO_FCR_EF       = 0x01, /*!< Enable FIFO's,                      > 0x00. */
    SIO_FCR_CRF      = 0x02, /*!< Clear Receive FIFO,                 > 0x00. */
    SIO_FCR_CTF      = 0x04, /*!< Clear Transmit FIFO,                > 0x00. */
    SIO_FCR_DMS      = 0x08, /*!< DMA Mode Select.
                                  Change status of RXRDY & TXRDY pins
                                  from mode 1 to mode 2 ,             > 0x00. */
    SIO_FCR_RES      = 0x10, /*!< Reserved.                                   */
    SIO_FCR_EBF64    = 0x20, /*!< Enable 64 Byte FIFO (16750 only),   > 0x00. */
    SIO_FCR_ITL_MASK = 0xC0, /*!< Interrupt Trigger Level Mask.               */
    SIO_FCR_ITL1     = 0x00, /*!< Interrupt Trigger Level 1 Byte,     = 0x00. */
    SIO_FCR_ITL4     = 0x40, /*!< Interrupt Trigger Level 4 Byte,     = 0x40. */
    SIO_FCR_ITL8     = 0x80, /*!< Interrupt Trigger Level 8 Byte,     = 0x80. */
    SIO_FCR_ITL14    = 0xC0  /*!< Interrupt Trigger Level 14 Byte,    = 0xC0. */
} sio_fcr_b_t;

/*! \enum sio_lcr_b_t
    Значения комбинаций битов регистра LCR.
    Доступ: rw.
*/
typedef enum SIO_LCR_B {
    SIO_LCR_WL_MASK    = 0x03, /*!< Word Length Mask.                                       */
    SIO_LCR_WL5        = 0x00, /*!< Word Length 5 bits,                             = 0x00. */
    SIO_LCR_WL6        = 0x01, /*!< Word Length 6 bits,                             = 0x01. */
    SIO_LCR_WL7        = 0x02, /*!< Word Length 7 bits,                             = 0x02. */
    SIO_LCR_WL8        = 0x03, /*!< Word Length 8 bits,                             = 0x03. */
    SIO_LCR_LSB        = 0x04, /*!< 0 = 1 Stop Bit,
                                    1 = 2 Stop bits for words of length 6,7 or 8 bits
                                          or 1.5 Stop Bits for Word lengths of 5 bits.      */
    SIO_LCR_P_MASK     = 0x38, /*!< parity Mask.                                            */
    SIO_LCR_NP         = 0x00, /*!< No parity ,                                     = 0x00. */
    SIO_LCR_OP         = 0x08, /*!< Odd parity ,                                    = 0x08. */
    SIO_LCR_EP         = 0x18, /*!< Even parity,                                    = 0x18. */
    SIO_LCR_HP         = 0x28, /*!< High parity,                                    = 0x28. */
    SIO_LCR_LP         = 0x38, /*!< Low parity,                                     = 0x38. */
    SIO_LCR_SBE        = 0x40, /*!< Set Break Enable,                               = 0x40. */
    SIO_LCR_DLAB       = 0x80  /*!< 0 = RB, THB, IER accessible,
                                    1 = DLLB, DLHB accessible.                              */
} sio_lcr_b_t;

/*! \enum sio_mcr_b_t
    Значения комбинаций битов регистра MCR.
    Доступ: rw.
*/
typedef enum SIO_MCR_B {
    SIO_MCR_FDTR = 0x01, /*!< Force Data Terminal Ready,             > 0x00. */
    SIO_MCR_FRS  = 0x02, /*!< Force Request to Send,                 > 0x00. */
    SIO_MCR_AO1  = 0x04, /*!< Aux Output 1,                          > 0x00. */
    SIO_MCR_AO2  = 0x08, /*!< Aux Output 2,                          > 0x00. */
    SIO_MCR_LM   = 0x10, /*!< LoopBack Mode ,                        > 0x00. */
    SIO_MCR_ACE  = 0x20, /*!< Autoflow Control Enabled (16750 only), > 0x00. */
    SIO_MCR_RES1 = 0x40, /*!< Reserved,                              > 0x00. */
    SIO_MCR_RES2 = 0x80  /*!< Reserved,                              > 0x00. */
} sio_mcr_b_t;

/*! \enum sio_lsr_b_t
    Значения комбинаций битов регистра LSR.
    Доступ: ro.
*/
typedef enum SIO_LSR_B {
    SIO_LSR_DR   = 0x01, /*!< Data Ready,                         > 0x00. */
    SIO_LSR_OE   = 0x02, /*!< Overrun Error,                      > 0x00. */
    SIO_LSR_PE   = 0x04, /*!< parity Error,                       > 0x00. */
    SIO_LSR_FE   = 0x08, /*!< Framing Error,                      > 0x00. */
    SIO_LSR_BI   = 0x10, /*!< Break Interrupt,                    > 0x00. */
    SIO_LSR_ETHR = 0x20, /*!< Empty Transmitter Holding Register, > 0x00. */
    SIO_LSR_EDHR = 0x40, /*!< Empty Data Holding Registers,       > 0x00. */
    SIO_LSR_ERF  = 0x80  /*!< Error in Received FIFO,             > 0x00. */
} sio_lsr_b_t;

/*! \enum sio_msr_b_t
    Значения комбинаций битов регистра MSR.
    Доступ: ro.
*/
typedef enum SIO_MSR_B {
    SIO_MSR_DCS  = 0x01, /*!< Delta Clear to Send,          > 0x00. */
    SIO_MSR_DDSR = 0x02, /*!< Delta Data Set Ready,         > 0x00. */
    SIO_MSR_TERI = 0x04, /*!< Trailing Edge Ring Indicator, > 0x00. */
    SIO_MSR_DDCD = 0x08, /*!< Delta Data Carrier Detect,    > 0x00. */
    SIO_MSR_CTS  = 0x10, /*!< Clear To Send,                > 0x00. */
    SIO_MSR_DSR  = 0x20, /*!< Data Set Ready,               > 0x00. */
    SIO_MSR_RI   = 0x40, /*!< Ring Indicator ,              > 0x00. */
    SIO_MSR_CD   = 0x80  /*!< Carrier Detect,               > 0x00. */
} sio_msr_b_t;

//--------------------------------------------------------------------------------------------------------//
/*** ЛОКАЛЬНЫЕ СТРУКТУРЫ ДАННЫХ ***/
/*! \struct sio_queue_t
    Очередь I/O.
*/
typedef struct SIO_QUEUE {
    char *data; /*!< Указатель на буфер очереди. */
    u16 size;   /*!< Максимальный размер буфера \a data очереди. */
    u16 in;     /* Index of where to store next character */
    u16 out;    /* Index of where to retrieve next character */
    u16 chars;  /* Count of characters in queue */
} sio_queue_t;

/*! \struct sio_addr_t
    Структура хранения адресов регистров UART.
*/
typedef struct SIO_ADDR {
    u16 base;    /*!< Базовый адрес порта (буферы TX, RX, DLLB). */
    u16 ier;     /*!< Базовый адрес IER/DLHB. */
    u16 iir_fcr; /*!< Базовый адрес IIR/FCR. */
    u16 lcr;     /*!< Базовый адрес LCR. */
    u16 mcr;     /*!< Базовый адрес MCR. */
    u16 lsr;     /*!< Базовый адрес LSR. */
    u16 msr;     /*!< Базовый адрес MSR. */
} sio_addr_t;

typedef enum FLAGS {
    F_BLOCK_MODE = 0x0001
} flags_t;

/*! \struct sio_uart_t
    Структура UART.
*/
typedef struct SIO_UART {
    sio_addr_t addr; /*!< Структура хранения адресов регистров UART. */
    sio_queue_t rx;  /*!< Приемная очередь. */
    sio_queue_t tx;  /*!< Передающая очередь. */
    int flags;       /*!< Флаги (например влаги открытия и т.п.). */
} sio_uart_t;

//--------------------------------------------------------------------------------------------------------//
/*** ЛОКАЛЬНЫЕ ПЕРЕМЕННЫЕ ***/

/* Базовые адреса, номера прерываний и маски прерываний портов. */
static const int bases[4] = {0x03F8, 0x02F8, 0x00, 0x03E8};     /*!< Базовые адреса портов COM1,COM2,COM_PGM,COM4. */
static const char intnums[4] = {0x0C, 0x0E, 0x14, 0x0C};        /*!< Номера прерываний от портов COM1,COM2,COM_PGM,COM4. */
static const int intmasks[4] = {0x0010, 0x0040, 0x0400, 0x010}; /*!< Маски прерываний от портов COM1,COM2,COM_PGM,COM4
                                                                     для контроллера прерываний. */

static sio_uart_t *uarts[4] = {0, 0, 0, 0};            /*!< Пользовательский массив структур конфигураций COM1,COM2,COM_PGM,COM4. */
static void (__interrupt *old_vecs[2])(void) = {0, 0}; /*!< Массив указателей на сохраненные векторы прерываний. */

#ifdef COM_PGM
static void (__interrupt *old_vec_pgm)(void);
static int old_ier = 0;
static int old_lcr = 0;
static int old_brd = 0;
#endif

//--------------------------------------------------------------------------------------------------------//
/*** ПАБЛИК ПЕРЕМЕННЫЕ ***/

int sioerrno = SIO_ERR_NONE; /*!< Код последней ошибки. */

//--------------------------------------------------------------------------------------------------------//
/*** ЛОКАЛЬНЫЕ ФУНКЦИИ ***/

/*! \fn void com_vce_isr(sio_com_t nport)
    Под-обработчик прерывания конкретного порта \a nport.
    \param nport Номер порта.
*/
static void com_vce_isr(sio_com_t nport)
{
    int r;
    while (!(SIO_IIR_IP & (r = inp(uarts[nport]->addr.iir_fcr)))) {

        switch (SIO_IIR_ID_MASK & r) {

            /* Modem Status Interrupt */
        case SIO_IIR_MSI:
            inp(uarts[nport]->addr.msr); // Just clear the interrupt
            break;

            /* Receiver Line Status Register */
        case SIO_IIR_RLSI:
            inp(uarts[nport]->addr.lsr); // Just clear the interrupt
            break;

            /* Empty Transmitter Holding Register */
        case SIO_IIR_THREI:
            if (SIO_LSR_ETHR & inp(uarts[nport]->addr.lsr)) {
                //передача 16 байт максимум
                //экономим и используем r как итератор
                for (r = 0; (r < 16) && (uarts[nport]->tx.chars > 0); ++r, uarts[nport]->tx.chars--) {
                    outp(uarts[nport]->addr.base, uarts[nport]->tx.data[uarts[nport]->tx.out++]);
                    if (uarts[nport]->tx.out == uarts[nport]->tx.size)
                        uarts[nport]->tx.out = 0;//количество байт в буфере tx_queue
                }
            }
            break;

            //case SIO_IIR_RDAI: /* Received Data Ready */
            //читаем из FIFO предположительно 14 байт, т.к. прерывание настроено через каждые 14 байт
            //   for (int i = 0; i < 14; ++i) {
            //        char c = inp(uarts[nport]->addr.base);//читаем символ из Rx
            //        if (uarts[nport]->rx.chars < uarts[nport]->rx.size) {
            //
            //            uarts[nport]->rx.data[uarts[nport]->rx.in++] = c;
            //
            //            if (uarts[nport]->rx.in == uarts[nport]->rx.size)
            //                uarts[nport]->rx.in = 0;
            //
            //            uarts[nport]->rx.chars++;
            //        }
            //        if (0 == (inp(uarts[nport]->addr.lsr) & SIO_LSR_DR))
            //            break; /*Data Ready > 0x00. */
            //   }
            //    break;


            /* Received Data Ready or Receive Data time out */
        case SIO_IIR_RDAI:
        case SIO_IIR_RDTO:
            while (SIO_LSR_DR & inp(uarts[nport]->addr.lsr)) { // Data Ready > 0x00
                //экономим и используем r как результат чтения
                r = inp(uarts[nport]->addr.base);//читаем символ из Rx
                if (uarts[nport]->rx.chars < uarts[nport]->rx.size) {
                    uarts[nport]->rx.data[uarts[nport]->rx.in++] = (char)r;
                    if (uarts[nport]->rx.in == uarts[nport]->rx.size)
                        uarts[nport]->rx.in = 0;
                    uarts[nport]->rx.chars++;
                }
            }
            break;

        default:;
        }//sw

    }//while
}

/*! \fn void void interrupt handler1_4(__CPPARGS)
    Обработчик прерываний портов COM1/COM4.
*/
static void __interrupt handler1_4(void)
{
    _enable();
    if (uarts[SIO_COM1]) { com_vce_isr(SIO_COM1); }
    if (uarts[SIO_COM4]) { com_vce_isr(SIO_COM4); }
    outpw(0xFF22, 0x000C);//сброс внешнего прерывания от INT0 UART, где 0x000C - EOI (End Of Interrupt)
}

/*! \fn void void interrupt handler2(__CPPARGS)
    Обработчик прерываний порта COM2.
*/
static void __interrupt handler2(void)
{
    _enable();
    if (uarts[SIO_COM2]) { com_vce_isr(SIO_COM2); }
    outpw(0xFF22, 0x000E);//сброс внешнего прерывания от INT2 UART, где 0x000E - EOI (End Of Interrupt)
}

#ifdef COM_PGM

/*! \fn void interrupt handler_pgm(__CPPARGS)
    Обработчик прерываний порта COM3 (для программирования, PGM).
*/
static void __interrupt handler_pgm(void)
{
    _enable();
    int r = inpw(uarts[SIO_COM_PGM]->addr.lsr);
    if ((0x000F & r) == 0) {
        //новое
        //передача
        if (0x0060 & r) {
            if ( 0 == uarts[SIO_COM_PGM]->tx.chars)
                outpw(uarts[SIO_COM_PGM]->addr.lcr, inpw(uarts[SIO_COM_PGM]->addr.lcr) & 0xF7FF);//конец передачи
            else {
                outp(uarts[SIO_COM_PGM]->addr.iir_fcr, uarts[SIO_COM_PGM]->tx.data[uarts[SIO_COM_PGM]->tx.out++]);
                if (uarts[SIO_COM_PGM]->tx.out == uarts[SIO_COM_PGM]->tx.size)
                    uarts[SIO_COM_PGM]->tx.out = 0;//количество байт в буффере tx_queue
                uarts[SIO_COM_PGM]->tx.chars--;
            }
        }
        //прием
        if (0x0010 & r) {
            r = inpw(uarts[SIO_COM_PGM]->addr.base);
            if (uarts[SIO_COM_PGM]->rx.chars < uarts[SIO_COM_PGM]->rx.size) {
                uarts[SIO_COM_PGM]->rx.data[uarts[SIO_COM_PGM]->rx.in++] = (char)r;
                if (uarts[SIO_COM_PGM]->rx.in == uarts[SIO_COM_PGM]->rx.size)
                    uarts[SIO_COM_PGM]->rx.in = 0;
                uarts[SIO_COM_PGM]->rx.chars++;
            }
        }
    }
    else
        outpw(uarts[SIO_COM_PGM]->addr.lsr, 0x00F0 & r);//если есть ошибки то их сбросить

    outpw(0xFF22, 0x0014);//сброс прерывания от внутреннего UART CPU , где 0x0014 - EOI (End Of Interrupt)
}

#endif //COM_PGM

//--------------------------------------------------------------------------------------------------------//
/*** ПАБЛИК ФУНКЦИИ ***/

/*! \fn int sio_open(sio_com_t nport, sio_mode_t mode, int tx_buf_size, int rx_buf_size)
    Открывает порт \a nport и создает для него буферы I/O:
    передающий размером \a tx_buf_size и приемный размером \a rx_buf_size.
    \param nport Номер порта.
    \param mode Режим открытия.
    \param tx_buf_size Размер передающего буфера порта, в байтах.
    \param rx_buf_size Размер приемного буфера порта, в байтах.
    \return -1 при ошибке.
*/
int sio_open(sio_com_t nport, sio_mode_t mode, int tx_buf_size, int rx_buf_size)
{
    if (uarts[nport]) {
        sioerrno = SIO_ERR_PORT_ALREADY_OPEN;
        return -1;
    }

    uarts[nport] = (sio_uart_t *)calloc(1, sizeof(sio_uart_t));//создать структуру
    if (!uarts[nport]) { //структура не создана?
        sioerrno = SIO_ERR_NOT_MEMORY;//ошибка
        return -1;
    }

    uarts[nport]->tx.data = (char *)calloc(1, tx_buf_size);//создать буфер TX
    if (uarts[nport]->tx.data) {//буфер создан?
        uarts[nport]->rx.data = (char *)calloc(1, rx_buf_size);//создать буфер RX
        if (!uarts[nport]->rx.data) {//буфер не создан?
            free(uarts[nport]->tx.data);//освободить буфере TX
            uarts[nport]->tx.data = 0;
        }
    }

    if (!(uarts[nport]->tx.data)) {//буфер RX или TX не созданы?
        free(uarts[nport]);
        uarts[nport] = 0;
        sioerrno = SIO_ERR_NOT_MEMORY;//ошибка
        return -1;
    }

    uarts[nport]->rx.size = rx_buf_size;//сохранить размер буфера
    uarts[nport]->tx.size = tx_buf_size;

    sioerrno = SIO_ERR_NONE;

    //установить режим
    if (SIO_BLOCK_MODE == mode) { uarts[nport]->flags |= F_BLOCK_MODE; }

#ifdef COM_PGM // конфигурация порта PGM

    if (SIO_COM_PGM == nport) {
        //адреса регистров регистров RX TX
        uarts[SIO_COM_PGM]->addr.base = 0xFF86;//Serial Port Receive Register
        uarts[SIO_COM_PGM]->addr.iir_fcr = 0xFF84;//Serial Port Transmit Register
        //адреса управляющих регистров
        uarts[SIO_COM_PGM]->addr.ier = 0xFF44; //Serial Port Interrupt Control Register
        uarts[SIO_COM_PGM]->addr.lcr = 0xFF80; //Serial Port Control Register
        uarts[SIO_COM_PGM]->addr.lsr = 0xFF82; //Serial Port Status Register
        uarts[SIO_COM_PGM]->addr.mcr = 0xFF88; //Serial Port Baud Rate Divisor Register
        _disable();
        //здесь хранятся старые значения регистров
        old_ier = inpw(uarts[SIO_COM_PGM]->addr.ier);
        old_lcr = inpw(uarts[SIO_COM_PGM]->addr.lcr);
        old_brd = inpw(uarts[SIO_COM_PGM]->addr.mcr);//сохранить старую скорость
        //прерывание
        outpw(0xFF28, (inpw(0xFF28) | intmasks[SIO_COM_PGM]));//запретить прерывание
        _enable();
        old_vec_pgm = _dos_getvect(intnums[SIO_COM_PGM]);//сохранить старый вектор прерывание
        _dos_setvect(intnums[SIO_COM_PGM], handler_pgm);//установить новое прерывание
        //конфигурация
        outpw(uarts[SIO_COM_PGM]->addr.lcr, 0x0417);//RXIE, WLGN, TMOD, RSIE, RMODE
        outpw(0xFF22, 0x0014);//сброс прерывание от UART
        outpw(uarts[SIO_COM_PGM]->addr.ier, 0x0017);// Interrupt enable
        outpw(0xFF28, inpw(0xFF28) & (~intmasks[SIO_COM_PGM]));//разрешить прерывание от UART
        return 0;
    }

#endif

    uarts[nport]->addr.base = bases[nport];//адреса регистров
    uarts[nport]->addr.ier = bases[nport] + SIO_OFFSET_IER;
    uarts[nport]->addr.iir_fcr = bases[nport] + SIO_OFFSET_IIR;
    uarts[nport]->addr.lcr = bases[nport] + SIO_OFFSET_LCR;
    uarts[nport]->addr.mcr = bases[nport] + SIO_OFFSET_MCR;
    uarts[nport]->addr.lsr = bases[nport] + SIO_OFFSET_LSR;
    uarts[nport]->addr.msr = bases[nport] + SIO_OFFSET_MSR;

    outp(uarts[nport]->addr.ier, 0x00);//disable interrupt
    if (inp(uarts[nport]->addr.ier)) {
        free(uarts[nport]->tx.data);//освободить буфер TX
        free(uarts[nport]->rx.data);//освободить буфер RX
        free(uarts[nport]);//освободить структуру
        uarts[nport] = 0;
        sioerrno = SIO_ERR_NO_UART;
        return -1;
    }

    //установить FIFO
    outp(uarts[nport]->addr.iir_fcr, SIO_FCR_CRF | SIO_FCR_CTF);//очистить FIFO
    outp(uarts[nport]->addr.iir_fcr, 0xC0);//установить trigger level = 14
    outp(uarts[nport]->addr.iir_fcr, SIO_FCR_EF);//включить

    /// ДОДЕЛАТЬ ПОТОМ!!!!
    /*if (var_2 == 1)
    {
        _disable();
        if ((inp(0x0178) & 0x80)==0)
        {
            if (inp( uarts[nport]->addr.lcr) != 0xBF)
            {
                outp(var_e, 0x00);
                outp(var_10, 0x18);
            }
        }
        _enable();
    }*/

    /* настроить прерывания uart */
    //Enable Received Data Available Interrupt, Enable Transmitter Holding Register Empty Interrupt
    outp(uarts[nport]->addr.ier, SIO_IER_ERDAI | SIO_IER_ETHREI);

    // COM1 и/или COM4
    if (((SIO_COM1 == nport) && (!uarts[SIO_COM4]))
            || ((SIO_COM4 == nport) && (!uarts[SIO_COM1]))) {

        _disable();
        outpw(0xFF22, 0x0C);////сброс внешние прерывание от INT0
        outpw(0xFF28, (inpw(0xFF28) | intmasks[nport]));//отключить внешнее прерывание
        _enable();

        old_vecs[0] = _dos_getvect(intnums[nport]);//сохранить старый вектор прерывания
        _dos_setvect(intnums[nport], handler1_4);//установить новое прерывание

        _disable();
        outpw(0xFF38, (inpw(0xFF38) | 0xCF));//прерывание инициировано нижним уровнем к высокому краю, Включите специальному полностью вложенному режиму INT0
        outpw(0xFF28, (inpw(0xFF28) & (~intmasks[nport]))); //включить внешнее прерывание от INT0
        _enable();

        uarts[nport]->flags |= F_BLOCK_MODE;
        return 0;
    }

    // COM2
    if (SIO_COM2 == nport) {
        _disable();
        outpw(0xFF22,0x0E);//сброс внешние прерывание от INT2
        outpw(0xFF28, (inpw(0xFF28) | intmasks[nport]));//отключить внешнее прерывание
        _enable();

        old_vecs[1] = _dos_getvect(intnums[nport]);//сохранить старый вектор прерывание
        _dos_setvect(intnums[nport],handler2);//установить новое прерывание

        _disable();
        outpw(0xFF28, (inpw(0xFF28) & (~intmasks[nport]))); //включить внешнее прерывание от INT2
        _enable();
    }
    return 0;
}

/*! \fn int sio_configure(sio_com_t nport, sio_speed_t baud, sio_parity_t parity, sio_databits_t databits, sio_stopbits_t stopbits)
    Конфигурирует открытый порт \a nport, т.е. устанавливает:
    скорость \a baud, паритет \a parity, кол-во бит данных \a databits, кол-во стоп-бит \a stopbits.
    \param nport Номер порта.
    \param baud Скорость.
    \param parity Паритет.
    \param databits Биты данных.
    \param stopbits Стоп биты.
    \return -1 при ошибке.
*/
int sio_configure(sio_com_t nport, sio_speed_t baud, sio_parity_t parity, sio_databits_t databits, sio_stopbits_t stopbits)
{
    if (!uarts[nport]) { return -1; }

    sioerrno = SIO_ERR_ILLEGAL_SETTING;//ошибка

#ifdef COM_PGM // конфигурация порта PGM

    if (SIO_COM_PGM == nport) {
        _disable();
        switch (baud) {//установка baud
        case SIO_BPS_50: outpw(uarts[SIO_COM_PGM]->addr.mcr, 24999); break;
        case SIO_BPS_300: outpw(uarts[SIO_COM_PGM]->addr.mcr, 4165); break;
        case SIO_BPS_600: outpw(uarts[SIO_COM_PGM]->addr.mcr, 2082); break;
        case SIO_BPS_2400: outpw(uarts[SIO_COM_PGM]->addr.mcr, 519); break;
        case SIO_BPS_4800: outpw(uarts[SIO_COM_PGM]->addr.mcr, 259); break;
        case SIO_BPS_9600: outpw(uarts[SIO_COM_PGM]->addr.mcr, 129); break;
        case SIO_BPS_19200: outpw(uarts[SIO_COM_PGM]->addr.mcr, 64); break;
        case SIO_BPS_38400: outpw(uarts[SIO_COM_PGM]->addr.mcr, 31); break;
        case SIO_BPS_57600: outpw(uarts[SIO_COM_PGM]->addr.mcr, 20); break;
        case SIO_BPS_115200: outpw(uarts[SIO_COM_PGM]->addr.mcr, 9); break;
        default:;
        }

        int r = inpw(uarts[SIO_COM_PGM]->addr.lcr) & 0xFF87; //очистка битов
        switch (parity) {//установка parity
        case SIO_PAR_NONE: break;
        case SIO_PAR_EVEN: r |= 0x0060; break;
        case SIO_PAR_ODD: r |= 0x0040; break;
        default: return -1;
        }

        switch (databits) {//установка databits
        case SIO_DATA7: break;
        case SIO_DATA8: r |= 0x0010; break;
        default: return -1;
        }

        switch (stopbits) {//установка stopbits
        case SIO_STOP1: break;
        case SIO_STOP2: r |= 0x0008; break;
        default:;
        }

        outpw (uarts[SIO_COM_PGM]->addr.lcr, r);//записать установки
        _enable();

        sioerrno = SIO_ERR_NONE;
        return 0;
    }

#endif

    _disable();
    outp(uarts[nport]->addr.lcr, (inp(uarts[nport]->addr.lcr) | SIO_LCR_DLAB));/*1 = DLLB, DLHB accessible.*/
    outpw(uarts[nport]->addr.base, baud);//установить скорость
    outp(uarts[nport]->addr.lcr, (inp(uarts[nport]->addr.lcr) & (~SIO_LCR_DLAB)));/*0 = RB accessible*/
    _enable();

    _disable();
    outp(uarts[nport]->addr.lcr, (parity | databits | stopbits));//остальные настройки порта
    _enable();

    sioerrno = SIO_ERR_NONE;
    return 0;
}

/*! \fn int sio_send(sio_com_t nport, const char *buf, int len)
    Передает в порт \a nport массив байт \a buf длиной \a len.
    \param nport Номер порта.
    \param buf Указатель на массив байт.
    \param len Кол-во байт для передачи.
    \return -1 при ошибке или кол-во переданных байт.
*/
int sio_send(sio_com_t nport, const char *buf, int len)
{
    if (!uarts[nport]) { return -1; }

    int bytes_written = 0;
    int is_block_mode = (F_BLOCK_MODE & uarts[nport]->flags);

    for (;;) {

        _disable();//откл прерывание

        /// Этот код в силе при отключенных прерываниях!!!
        int bytes_to_write = ((uarts[nport]->tx.chars + len) > uarts[nport]->tx.size) ?
                    (uarts[nport]->tx.size - uarts[nport]->tx.chars) : (len);

        if (bytes_to_write) {

            //сколько копировать в буфер? это на границе кольцевого буфера?
            int sizecpy = ((uarts[nport]->tx.in + bytes_to_write) <= uarts[nport]->tx.size) ?
                        (bytes_to_write) : (uarts[nport]->tx.size - uarts[nport]->tx.in);
            //копируем до границы кольцевого буфера
            memcpy(uarts[nport]->tx.data + uarts[nport]->tx.in, buf, sizecpy);

            if (sizecpy < bytes_to_write) {//это не все?
                sizecpy = bytes_to_write - sizecpy;//вычисляем остаток
                uarts[nport]->tx.in = 0;// начало буфера
                //копируем остаток в начало кольцевого буфера
                memcpy(uarts[nport]->tx.data, buf + (bytes_to_write - sizecpy), sizecpy);
            }
            uarts[nport]->tx.in += sizecpy; //указатель на пустую ячейку кольцевого буфера
            uarts[nport]->tx.chars += bytes_to_write;  //добавляем количество байт для передачи
            //указатель на пустую ячейку кольцевого буфера за пределами размера буфера?
            if (uarts[nport]->tx.in >= uarts[nport]->tx.size)
                uarts[nport]->tx.in = 0;

            //запуск прерывания по передаче
#ifdef COM_PGM // передача порт PGM
            if (SIO_COM_PGM == nport) {
                if (!(inpw(uarts[SIO_COM_PGM]->addr.lcr) & 0x0800))
                    outpw(uarts[SIO_COM_PGM]->addr.lcr, inpw(uarts[SIO_COM_PGM]->addr.lcr) | 0x0800);
            }
            else {
#endif
                if (inp(uarts[nport]->addr.lsr) & 0x20) {//нет передачи?
                    uarts[nport]->tx.chars--;
                    // передача байта
                    outp(uarts[nport]->addr.base, uarts[nport]->tx.data[uarts[nport]->tx.out++]);
                    //указатель передачи за границей буфера?
                    if (uarts[nport]->tx.out == uarts[nport]->tx.size)
                        uarts[nport]->tx.out = 0;
                }
#ifdef COM_PGM
            }
#endif

        }//bytes_to_write > 0

        _enable();//вкл прерывание

        bytes_written += bytes_to_write;
        len -= bytes_to_write;

        if (is_block_mode) {
            if (!len)
                break;
        }
        else
            break;
    }
    return bytes_written;
}

/*! \fn int sio_recv(sio_com_t nport, char *buf, int len)
    Принимает из порта \a nport массив байт \a buf длиной \a len.
    \param nport Номер порта.
    \param buf Указатель на массив байт.
    \param len Кол-во байт для передачи.
    \return -1 при ошибке.
*/
int sio_recv(sio_com_t nport, char *buf, int len)
{
    if (!uarts[nport]) { return -1; }

    int bytes_readed = 0;
    int is_block_mode = (F_BLOCK_MODE & uarts[nport]->flags);

    for (;;) {

        _disable();//откл прерывание

        /// Этот код в силе при отключенных прерываниях!!!
        int bytes_to_read = (uarts[nport]->rx.chars > len) ? (len) : (uarts[nport]->rx.chars);

        if (bytes_to_read) {

            //сколько копировать в буфер? эта на границе кольцевого буфера?
            int sizecpy = ((uarts[nport]->rx.out + bytes_to_read) <=  uarts[nport]->rx.size) ?
                        (bytes_to_read) : (uarts[nport]->rx.size - uarts[nport]->rx.out);
            //копируем до границе кольцевого буфера
            memcpy(buf, uarts[nport]->rx.data + uarts[nport]->rx.out, sizecpy);
            if (sizecpy < bytes_to_read) {//это не все?
                sizecpy = bytes_to_read - sizecpy;//вычисляем остаток
                uarts[nport]->rx.out = 0;// начало буфера
                //копируем остаток в начало кольцевого буфера
                memcpy(buf + (bytes_to_read - sizecpy), uarts[nport]->rx.data, sizecpy);
            }
            uarts[nport]->rx.out += sizecpy; //указатель на пустую ячейку кольцевого буфера
            uarts[nport]->rx.chars -= bytes_to_read;  //количество байт в приемном буфере

            //указатель на ячейку кольцевого буфера за пределами размера буфера?
            if (uarts[nport]->rx.out >= uarts[nport]->rx.size)
                uarts[nport]->rx.out = 0;

        }//bytes_to_read > 0

        _enable();//вкл прерывание

        bytes_readed += bytes_to_read;
        len -= bytes_to_read;

        if (is_block_mode) {
            if (!len)
                break;
        }
        else
            break;
    }
    return bytes_readed;
}

/*! \fn int sio_clear(sio_com_t nport, sio_dir_t dir)
    Очишает передающую/принимающую \a dir очередь порта \a nport.
    \param nport Номер порта.
    \param dir Направление прием/передача (можно по ИЛИ их установить!).
    \return -1 при ошибке.
*/
int sio_clear(sio_com_t nport, sio_dir_t dir)
{
    if (uarts[nport] /*sio_exists(nport)*/ ) {
        if (SIO_TX_DIRECTION & dir) {
            uarts[nport]->tx.in = 0;
            uarts[nport]->tx.out = 0;
            uarts[nport]->tx.chars = 0;
        }
        if (SIO_RX_DIRECTION & dir) {
            uarts[nport]->rx.in = 0;
            uarts[nport]->rx.out = 0;
            uarts[nport]->rx.chars = 0;
        }
        return 0;
    }
    return -1;
}

/*! \fn int sio_rx_available(sio_com_t nport)
    Возвращает кол-во байт порта \a nport доступных для чтения.
    \param nport Номер порта.
    \return Кол-во доступных для чтения байт.
*/
int sio_rx_available(sio_com_t nport)
{
    return (uarts[nport]) ? (uarts[nport]->rx.chars) : (0);
}

/*! \fn void sio_close(sio_com_t nport)
    Закрывает порт \a nport.
    \param nport Номер порта.
*/
void sio_close(sio_com_t nport)
{
    if (!uarts[nport]) { return; }

    int is_block_mode = (F_BLOCK_MODE & uarts[nport]->flags);

    //ждем конца передачи
    if (is_block_mode) {
        while (uarts[nport]->tx.chars > 0) {}
        while (!(inpw(uarts[SIO_COM_PGM]->addr.lsr) & SIO_LSR_ETHR)) {}
    }

    ///sio_clear(nport, SIO_TX_DIRECTION | SIO_RX_DIRECTION);/// ???

#ifdef COM_PGM // закрыть порт PGM
    if (SIO_COM_PGM == nport) {
        _disable();
        outpw(0xFF28, (inpw(0xFF28) | intmasks[SIO_COM_PGM]));//запретить прерывание
        //прерывание
        _dos_setvect(intnums[SIO_COM_PGM], old_vec_pgm);//вернуть старое прерывание
        //вернуть старою конфигурацию
        outpw(uarts[SIO_COM_PGM]->addr.ier, old_ier);// Interrupt enable
        outpw(uarts[SIO_COM_PGM]->addr.mcr, old_brd);// старая скорость
        outpw(uarts[SIO_COM_PGM]->addr.lcr, old_lcr);// старая конфигурация
        outpw(0xFF22, 0x0014);//сброс прерывание от UART
        outpw(0xFF28, inpw(0xFF28) & (~intmasks[SIO_COM_PGM]));//разрешить прерывание от UART
        _enable();
    }
    else {
#endif
        //отключить прерывание uart
        outp(uarts[nport]->addr.ier, 0x00);//disable interrupt
        outp(uarts[nport]->addr.iir_fcr, (inpw(uarts[nport]->addr.iir_fcr) & (~SIO_FCR_EF)));//отключить FIFO
        //COM1 и/или COM4
        if (((SIO_COM1 == nport) && (!uarts[SIO_COM4]))
                || ((SIO_COM4 == nport) && (!uarts[SIO_COM1]))) {//это COM1 и COM4 не открыт или это COM4 и COM1 не открыт?
            //отключить внешнее прерывание от микросхемы uart
            _disable();
            outpw(0xFF28, (inpw(0xFF28) | intmasks[nport]));//отключить внешнее прерывание
            _enable();
            if (old_vecs[0])
                _dos_setvect(intnums[nport], old_vecs[0]);//вернуть старое прерывание
        }
        // COM2
        if (SIO_COM2 == nport) {
            //отключить внешнее прерывание от микросхемы uart
            _disable();
            outpw(0xFF28, (inpw(0xFF28) | intmasks[nport]));//отключить внешнее прерывание
            _enable();
            if (old_vecs[1])
                _dos_setvect(intnums[nport], old_vecs[1]);//вернуть старое прерывание
        }
#ifdef COM_PGM
    }
#endif

    free(uarts[nport]->tx.data);//освободить буфер TX
    free(uarts[nport]->rx.data);//освободить буфер RX
    free(uarts[nport]);//освободить структуру
    uarts[nport] = 0;
}

//TEST
/*
void sio_print_structs_size(void)
{
    printf("= SIO =\n");
    printf("- sio_queue_t : %d\n", sizeof(sio_queue_t));
    printf("- sio_addr_t : %d\n", sizeof(sio_addr_t));
    printf("- sio_uart_t : %d\n", sizeof(sio_uart_t));

}
*/
