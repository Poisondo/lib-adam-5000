/*********************************************************************************************
Project:
Version:
Date: 24.03.2011
Author: Oleg Diyakonov, Denis Shienkov
Company:
Comments: A library for work with serial ports in your controllers ADAM 5510.
**********************************************************************************************/

/*! \file sio.cpp
 *
 * Abbreviation of the module (file) "sio" - Serial Input Output.
 *
 * This module implements an interface to access and work with serial
 * ports COM1 - COM4 PLC ADAM 5510.
 */

#include "sio.h"

#include <dos.h>
#include <mem.h>
#include <malloc.h>
#include <conio.h>


//--------------------------------------------------------------------------------------------------------//
/*** Private enums of addresses and of bits of registers ***/

/*!
 * Offsets a UART registers relative to the base address.
 * By default, a typical UART has 12 registers.
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

/*!
 * The values of the combinations of bits of the register IER.
 * Access: rw, when DLAB = 0.
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

/*!
 * The values of the combinations of bits of the register IIR.
 * Access: ro.
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

/*!
 * The values of the combinations of bits of the register FCR.
 * Access: wo.
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

/*!
 * The values of the combinations of bits of the register LCR.
 * Access: rw.
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

/*!
 * The values of the combinations of bits of the register MCR.
 * Access: rw.
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

/*!
 * The values of the combinations of bits of the register LSR.
 * Access: ro.
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

/*!
 * The values of the combinations of bits of the register MSR.
 * Access: ro.
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
/*** Private data structures ***/

/*!
 * I/O queue.
 *
 * Each direction of transmission in each a UART
 * has its own separate queue.
 */
typedef struct SIO_QUEUE {
    char *data; /*!< Queue data pointer. */
    u16 size;   /*!< Maximum size of data buffer a queue. */
    u16 in;     /*!< Index of where to store next character. */
    u16 out;    /*!< Index of where to retrieve next character. */
    u16 chars;  /*!< Count of characters in queue. */
} sio_queue_t;

/*!
 * The structure of the store address registers of each UART.
 */
typedef struct SIO_ADDR {
    u16 base;    /*!< Base address a UART (buffers TX, RX, DLLB). */
    u16 ier;     /*!< Base address IER/DLHB. */
    u16 iir_fcr; /*!< Base address IIR/FCR. */
    u16 lcr;     /*!< Base address LCR. */
    u16 mcr;     /*!< Base address MCR. */
    u16 lsr;     /*!< Base address LSR. */
    u16 msr;     /*!< Base address MSR. */
} sio_addr_t;

/*!
 * Internal UART flags.
 */
typedef enum FLAGS {
    F_BLOCK_MODE = 0x0001 /*!< The flag state, which means that the port is open in blocking mode. */
} flags_t;

/*!
 * UART structure.
*/
typedef struct SIO_UART {
    sio_addr_t addr; /*!< Structure in which stored UART addresses. */
    sio_queue_t rx;  /*!< Input queue. */
    sio_queue_t tx;  /*!< Output queue. */
    int flags;       /*!< Flags (eg. open mode flag, and etc. */
} sio_uart_t;

//--------------------------------------------------------------------------------------------------------//
/*** Private variables ***/

/* Base addresses, interrupt numbers and masks. */

/*!
 * COM1,COM2,COM_PGM,COM4 base addresses.
 */
static const int bases[4] = {0x03F8, 0x02F8, 0x00, 0x03E8};
/*!
 * COM1,COM2,COM_PGM,COM4 interrupts.
 */
static const char intnums[4] = {0x0C, 0x0E, 0x14, 0x0C};
/*!
 * COM1,COM2,COM_PGM,COM4 interrupt masks
 * for interrupt controller.
 */
static const int intmasks[4] = {0x0010, 0x0040, 0x0400, 0x010};
/*!
 * An array of pointers to configuration COM1,COM2,COM_PGM,COM4.
 */
static sio_uart_t *uarts[4] = {0, 0, 0, 0};
/*!
 * An array of pointers to the old interrupt vectors.
 */
static void (__interrupt *old_vecs[2])(void) = {0, 0};

#ifdef COM_PGM
/*!
 * Pointer to the PGM old interrupt vector.
 */
static void (__interrupt *old_vec_pgm)(void);
/*!
 * PGM old IER state.
 */
static int old_ier = 0;
/*!
 * PGM old LCR state.
 */
static int old_lcr = 0;
/*!
 * PGM old state.
 */
static int old_brd = 0;
#endif

//--------------------------------------------------------------------------------------------------------//
/*** Public variables ***/

int sioerrno = SIO_ERR_NONE;

//--------------------------------------------------------------------------------------------------------//
/*** Private functions� ***/

/*!
 * Interrupt sub-handler a concrete of port \a nport.
 * \param nport Port number as sio_com_t.
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
                // Transfer a maximum 16 byte.
                // Use r variable as iterator (for economy).
                for (r = 0; (r < 16) && (uarts[nport]->tx.chars > 0); ++r, uarts[nport]->tx.chars--) {
                    outp(uarts[nport]->addr.base, uarts[nport]->tx.data[uarts[nport]->tx.out++]);
                    if (uarts[nport]->tx.out == uarts[nport]->tx.size) {
                        uarts[nport]->tx.out = 0; // Number of bytes in tx queue.
                    }
                }
            }
            break;

            //case SIO_IIR_RDAI: /* Received Data Ready */
            // Read from the FIFO 14-byte expected, as interrupt is set to every 14 bytes.
            //   for (int i = 0; i < 14; ++i) {
            //        char c = inp(uarts[nport]->addr.base); // Read byte from UART.
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
                // Use r variable as read result (for economy).
                r = inp(uarts[nport]->addr.base); // Read byte from UART.
                if (uarts[nport]->rx.chars < uarts[nport]->rx.size) {
                    uarts[nport]->rx.data[uarts[nport]->rx.in++] = (char)r;
                    if (uarts[nport]->rx.in == uarts[nport]->rx.size) {
                        uarts[nport]->rx.in = 0;
                    }
                    uarts[nport]->rx.chars++;
                }
            }
            break;

        default:;
        }//sw

    }//while
}

/*!
 * COM1/COM4 interrupt handler.
 */
static void __interrupt handler1_4(void)
{
    _enable();
    if (uarts[SIO_COM1]) {
        com_vce_isr(SIO_COM1);
    }
    if (uarts[SIO_COM4]) {
        com_vce_isr(SIO_COM4);
    }
    // Reset the external interrupt INT0 UART,
    // where 0x000C - EOI (End Of Interrupt)
    outpw(0xFF22, 0x000C);
}

/*!
 * COM2 interrupt handler.
 */
static void __interrupt handler2(void)
{
    _enable();
    if (uarts[SIO_COM2]) {
        com_vce_isr(SIO_COM2);
    }
    // Reset the external interrupt INT2 UART,
    // where 0x000E - EOI (End Of Interrupt)
    outpw(0xFF22, 0x000E);
}

#ifdef COM_PGM

/*!
 * PGM interrupt handler.
 */
static void __interrupt handler_pgm(void)
{
    _enable();
    int r = inpw(uarts[SIO_COM_PGM]->addr.lsr);
    if ((0x000F & r) == 0) {
        //новое
        //передача
        if (0x0060 & r) {
            if ( 0 == uarts[SIO_COM_PGM]->tx.chars) {
                // Enf of transmission.
                outpw(uarts[SIO_COM_PGM]->addr.lcr, inpw(uarts[SIO_COM_PGM]->addr.lcr) & 0xF7FF);
            } else {
                outp(uarts[SIO_COM_PGM]->addr.iir_fcr, uarts[SIO_COM_PGM]->tx.data[uarts[SIO_COM_PGM]->tx.out++]);
                if (uarts[SIO_COM_PGM]->tx.out == uarts[SIO_COM_PGM]->tx.size) {
                    uarts[SIO_COM_PGM]->tx.out = 0; // Number of bytes in tx queue.
                }
                uarts[SIO_COM_PGM]->tx.chars--;
            }
        }
        // Receive.
        if (0x0010 & r) {
            r = inpw(uarts[SIO_COM_PGM]->addr.base);
            if (uarts[SIO_COM_PGM]->rx.chars < uarts[SIO_COM_PGM]->rx.size) {
                uarts[SIO_COM_PGM]->rx.data[uarts[SIO_COM_PGM]->rx.in++] = (char)r;
                if (uarts[SIO_COM_PGM]->rx.in == uarts[SIO_COM_PGM]->rx.size) {
                    uarts[SIO_COM_PGM]->rx.in = 0;
                }
                uarts[SIO_COM_PGM]->rx.chars++;
            }
        }
    } else {
        //If have errors then reset it.
        outpw(uarts[SIO_COM_PGM]->addr.lsr, 0x00F0 & r);
    }
    // Reset interrupt from the internal UART CPU,
    // where 0x0014 - EOI (End Of Interrupt)
    outpw(0xFF22, 0x0014);
}

#endif //COM_PGM

//--------------------------------------------------------------------------------------------------------//
/*** Public functions ***/

/*!
 * Opens port \a nport with the desired \a mode.
 * In the process of opening create internal an input and an
 * output buffer of the specified size \a tx_buf_size and \a rx_buf_size.
 * \param nport Port number as sio_com_t.
 * \param mode Open mode as sio_mode_t.
 * \param tx_buf_size Size of the output buffer of port, in bytes.
 * \param rx_buf_size Size of the input buffer of port, in bytes.
 * \return -1 on error.
 */
int sio_open(sio_com_t nport, sio_mode_t mode, int tx_buf_size, int rx_buf_size)
{
    if (uarts[nport]) {
        sioerrno = SIO_ERR_PORT_ALREADY_OPEN;
        return -1;
    }

    // Create internel port structure.
    uarts[nport] = (sio_uart_t *)calloc(1, sizeof(sio_uart_t));
    if (!uarts[nport]) {
        sioerrno = SIO_ERR_NOT_MEMORY;
        return -1;
    }

    // Create output port buffer.
    uarts[nport]->tx.data = (char *)calloc(1, tx_buf_size);
    if (uarts[nport]->tx.data) {
        // Create input port buffer.
        uarts[nport]->rx.data = (char *)calloc(1, rx_buf_size);
        if (!uarts[nport]->rx.data) {
            free(uarts[nport]->tx.data);
            uarts[nport]->tx.data = 0;
        }
    }

    // Check existing a input and output buffers.
    if (!(uarts[nport]->tx.data)) {
        free(uarts[nport]);
        uarts[nport] = 0;
        sioerrno = SIO_ERR_NOT_MEMORY;
        return -1;
    }

    // Save buffers size.
    uarts[nport]->rx.size = rx_buf_size;
    uarts[nport]->tx.size = tx_buf_size;

    sioerrno = SIO_ERR_NONE;

    // Set open mode.
    if (SIO_BLOCK_MODE == mode) {
        uarts[nport]->flags |= F_BLOCK_MODE;
    }

#ifdef COM_PGM // PGM configure.

    if (SIO_COM_PGM == nport) {
        // Addresses of RX and TX registers.
        uarts[SIO_COM_PGM]->addr.base = 0xFF86;    // Receive Register.
        uarts[SIO_COM_PGM]->addr.iir_fcr = 0xFF84; // Transmit Register.
        // Addresses of control regisrets.
        uarts[SIO_COM_PGM]->addr.ier = 0xFF44; // Interrupt Control Register.
        uarts[SIO_COM_PGM]->addr.lcr = 0xFF80; // Control Register.
        uarts[SIO_COM_PGM]->addr.lsr = 0xFF82; // Status Register.
        uarts[SIO_COM_PGM]->addr.mcr = 0xFF88; // Baud Rate Divisor Register.
        _disable();
        // Save old registers state.
        old_ier = inpw(uarts[SIO_COM_PGM]->addr.ier);
        old_lcr = inpw(uarts[SIO_COM_PGM]->addr.lcr);
        old_brd = inpw(uarts[SIO_COM_PGM]->addr.mcr); // Old baud rate.
        // Interrupts.
        outpw(0xFF28, (inpw(0xFF28) | intmasks[SIO_COM_PGM])); // Disable UART interrupts.
        _enable();
        old_vec_pgm = _dos_getvect(intnums[SIO_COM_PGM]); // Save old interrupt vector.
        _dos_setvect(intnums[SIO_COM_PGM], handler_pgm);  // Set new interrupt vector.
        // Configuring.
        outpw(uarts[SIO_COM_PGM]->addr.lcr, 0x0417); // RXIE, WLGN, TMOD, RSIE, RMODE.
        outpw(0xFF22, 0x0014);// Reset UART interrupts.
        outpw(uarts[SIO_COM_PGM]->addr.ier, 0x0017);// Interrupt enable
        outpw(0xFF28, inpw(0xFF28) & (~intmasks[SIO_COM_PGM]));// Enable UART interrupts.
        return 0;
    }

#endif

    // "Standard" UART configure.
    uarts[nport]->addr.base = bases[nport];
    uarts[nport]->addr.ier = bases[nport] + SIO_OFFSET_IER;
    uarts[nport]->addr.iir_fcr = bases[nport] + SIO_OFFSET_IIR;
    uarts[nport]->addr.lcr = bases[nport] + SIO_OFFSET_LCR;
    uarts[nport]->addr.mcr = bases[nport] + SIO_OFFSET_MCR;
    uarts[nport]->addr.lsr = bases[nport] + SIO_OFFSET_LSR;
    uarts[nport]->addr.msr = bases[nport] + SIO_OFFSET_MSR;

    outp(uarts[nport]->addr.ier, 0x00);// Disable interrupt.
    // Check UART exists.
    if (inp(uarts[nport]->addr.ier)) {
        free(uarts[nport]->tx.data);
        free(uarts[nport]->rx.data);
        free(uarts[nport]);
        uarts[nport] = 0;
        sioerrno = SIO_ERR_NO_UART;
        return -1;
    }

    // Configure FIFO.
    outp(uarts[nport]->addr.iir_fcr, SIO_FCR_CRF | SIO_FCR_CTF); // Clear FIFO.
    outp(uarts[nport]->addr.iir_fcr, 0xC0);                      // Set FIFO's trigger level = 14.
    outp(uarts[nport]->addr.iir_fcr, SIO_FCR_EF);                // Enable FIFO.

    // FIXME: Finish in the future.
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

    /* Configuring "standard" UART interrupts. */

    // Enable Received Data Available Interrupt,
    // Enable Transmitter Holding Register Empty Interrupt
    outp(uarts[nport]->addr.ier, SIO_IER_ERDAI | SIO_IER_ETHREI);

    // COM1 and/or COM4
    if (((SIO_COM1 == nport) && (!uarts[SIO_COM4]))
            || ((SIO_COM4 == nport) && (!uarts[SIO_COM1]))) {

        _disable();
        outpw(0xFF22, 0x0C); // Reset external interrups from  INT0.
        outpw(0xFF28, (inpw(0xFF28) | intmasks[nport])); // Disable external interupt.
        _enable();

        old_vecs[0] = _dos_getvect(intnums[nport]); // Save old interrupt vector.
        _dos_setvect(intnums[nport], handler1_4);   // Set new interrupt vector.

        _disable();
        // Interrupt initiated by the lower level to high end,
        // including a special fully nested mode INT0
        outpw(0xFF38, (inpw(0xFF38) | 0xCF));
        outpw(0xFF28, (inpw(0xFF28) & (~intmasks[nport]))); // Enable external interrups from INT0.
        _enable();

        uarts[nport]->flags |= F_BLOCK_MODE;
        return 0;
    }

    // COM2
    if (SIO_COM2 == nport) {
        _disable();
        outpw(0xFF22,0x0E); // Reset external interrups from  INT2.
        outpw(0xFF28, (inpw(0xFF28) | intmasks[nport])); // Disable external interupt.
        _enable();

        old_vecs[1] = _dos_getvect(intnums[nport]); // Save old interrupt vector.
        _dos_setvect(intnums[nport],handler2);      // Set new interrupt vector.

        _disable();
        outpw(0xFF28, (inpw(0xFF28) & (~intmasks[nport]))); // Enable external interrups from INT2.
        _enable();
    }
    return 0;
}

/*!
 * Configures an open port \a nport, and sets desired
 * rate \a baud, parity \a parity, number of data bits \a databits,
 * number of stop bits \a stopbits.
 * \param nport Port number as sio_com_t.
 * \param baud Desired rate as sio_speed_t.
 * \param parity Desired parity as sio_parity_t.
 * \param databits Desired data bits as sio_databits_t.
 * \param stopbits Desired stop bits as sio_stopbits_t.
 * \return -1 on error.
 */
int sio_configure(sio_com_t nport, sio_speed_t baud, sio_parity_t parity, sio_databits_t databits, sio_stopbits_t stopbits)
{
    if (!uarts[nport]) {
        return -1;
    }

    sioerrno = SIO_ERR_ILLEGAL_SETTING; // Forse error set.

#ifdef COM_PGM // PGM configure.

    if (SIO_COM_PGM == nport) {
        _disable();
        switch (baud) {
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

        int r = inpw(uarts[SIO_COM_PGM]->addr.lcr) & 0xFF87; // Clear bits.
        switch (parity) {
        case SIO_PAR_NONE: break;
        case SIO_PAR_EVEN: r |= 0x0060; break;
        case SIO_PAR_ODD: r |= 0x0040; break;
        default: return -1;
        }

        switch (databits) {
        case SIO_DATA7: break;
        case SIO_DATA8: r |= 0x0010; break;
        default: return -1;
        }

        switch (stopbits) {
        case SIO_STOP1: break;
        case SIO_STOP2: r |= 0x0008; break;
        default:;
        }

        outpw (uarts[SIO_COM_PGM]->addr.lcr, r); // Set new parameters.
        _enable();

        sioerrno = SIO_ERR_NONE;
        return 0;
    }

#endif

    // Configure "standard" UART.
    _disable();
    // 1 = DLLB, DLHB accessible.
    outp(uarts[nport]->addr.lcr, (inp(uarts[nport]->addr.lcr) | SIO_LCR_DLAB));
    // Set baud rate.
    outpw(uarts[nport]->addr.base, baud);
    // 0 = RB accessible.
    outp(uarts[nport]->addr.lcr, (inp(uarts[nport]->addr.lcr) & (~SIO_LCR_DLAB)));
    _enable();

    _disable();
    // Set other's parameters.
    outp(uarts[nport]->addr.lcr, (parity | databits | stopbits));
    _enable();

    sioerrno = SIO_ERR_NONE;
    return 0;
}

/*!
 * Send to port \a nport byte array \a buf of length \a len.
 * \param nport Port number as sio_com_t.
 * \param buf A pointer to an array of bytes.
 * \param len Number of bytes to transfer.
 * \return -1 on error or number of bytes transferred.
 */
int sio_send(sio_com_t nport, const char *buf, int len)
{
    if (!uarts[nport]) { return -1; }

    int bytes_written = 0;
    int is_block_mode = (F_BLOCK_MODE & uarts[nport]->flags);

    for (;;) {

        _disable();

        // This code is valid for disabling interrupts!!!
        int bytes_to_write = ((uarts[nport]->tx.chars + len) > uarts[nport]->tx.size) ?
                    (uarts[nport]->tx.size - uarts[nport]->tx.chars) : (len);

        if (bytes_to_write) {

            // How many to copy to the ring buffer?
            // It is on the border of the ring buffer?
            int sizecpy = ((uarts[nport]->tx.in + bytes_to_write) <= uarts[nport]->tx.size) ?
                        (bytes_to_write) : (uarts[nport]->tx.size - uarts[nport]->tx.in);

            // Copy to the boundary of the ring buffer
            memcpy(uarts[nport]->tx.data + uarts[nport]->tx.in, buf, sizecpy);

            if (sizecpy < bytes_to_write) { // This is not all?
                sizecpy = bytes_to_write - sizecpy; // Compute the remainder.
                uarts[nport]->tx.in = 0; // Beginning of the buffer.
                // Copy the remainder to the beginning of the ring buffer.
                memcpy(uarts[nport]->tx.data, buf + (bytes_to_write - sizecpy), sizecpy);
            }

            uarts[nport]->tx.in += sizecpy; // A pointer to an next empty cell, the ring buffer.
            uarts[nport]->tx.chars += bytes_to_write; // Increment number of bytes for transfer.

            // A pointer to an empty cell, the ring buffer outside the buffer size?
            if (uarts[nport]->tx.in >= uarts[nport]->tx.size) {
                uarts[nport]->tx.in = 0;
            }

            // Start interrupt for transfer.
#ifdef COM_PGM // PGM transfer.
            if (SIO_COM_PGM == nport) {
                if (!(inpw(uarts[SIO_COM_PGM]->addr.lcr) & 0x0800)) {
                    outpw(uarts[SIO_COM_PGM]->addr.lcr, inpw(uarts[SIO_COM_PGM]->addr.lcr) | 0x0800);
                }
            } else {
#endif
                if (inp(uarts[nport]->addr.lsr) & 0x20) { // No transmission?
                    uarts[nport]->tx.chars--;
                    // Byte transfer.
                    outp(uarts[nport]->addr.base, uarts[nport]->tx.data[uarts[nport]->tx.out++]);
                    // Transmission pointer a buffer is out boundary?
                    if (uarts[nport]->tx.out == uarts[nport]->tx.size) {
                        uarts[nport]->tx.out = 0;
                    }
                }
#ifdef COM_PGM
            }
#endif

        }//bytes_to_write > 0

        _enable();

        bytes_written += bytes_to_write;
        len -= bytes_to_write;

        if (is_block_mode) {
            if (!len) {
                break;
            }
        } else {
            break;
        }
    }
    return bytes_written;
}

/*!
 * Received from the port \a nport byte array \a buf of length \a len.
 * \param nport Port number as sio_com_t.
 * \param buf A pointer to an array of bytes.
 * \param len Number of bytes to receive.
 * \return -1 on error.
 */
int sio_recv(sio_com_t nport, char *buf, int len)
{
    if (!uarts[nport]) {
        return -1;
    }

    int bytes_readed = 0;
    int is_block_mode = (F_BLOCK_MODE & uarts[nport]->flags);

    for (;;) {

        _disable();

        // This code is valid for disabling interrupts!!!
        int bytes_to_read = (uarts[nport]->rx.chars > len) ? (len) : (uarts[nport]->rx.chars);

        if (bytes_to_read) {

            // How many to copy to the ring buffer?
            // It is on the border of the ring buffer?
            int sizecpy = ((uarts[nport]->rx.out + bytes_to_read) <=  uarts[nport]->rx.size) ?
                        (bytes_to_read) : (uarts[nport]->rx.size - uarts[nport]->rx.out);

            // Copy to the boundary of the ring buffer.
            memcpy(buf, uarts[nport]->rx.data + uarts[nport]->rx.out, sizecpy);

            if (sizecpy < bytes_to_read) { // This is not all?
                sizecpy = bytes_to_read - sizecpy; // Compute the remainder.
                uarts[nport]->rx.out = 0; // Beginning of the buffer.
                // Copy the remainder to the beginning of the ring buffer.
                memcpy(buf + (bytes_to_read - sizecpy), uarts[nport]->rx.data, sizecpy);
            }

            uarts[nport]->rx.out += sizecpy; // A pointer to an next empty cell, the ring buffer.
            uarts[nport]->rx.chars -= bytes_to_read; // Increment number of bytes for transfer.

            // A pointer to an empty cell, the ring buffer outside the buffer size?
            if (uarts[nport]->rx.out >= uarts[nport]->rx.size) {
                uarts[nport]->rx.out = 0;
            }

        }//bytes_to_read > 0

        _enable();

        bytes_readed += bytes_to_read;
        len -= bytes_to_read;

        if (is_block_mode) {
            if (!len) {
                break;
            }
        } else {
            break;
        }
    }
    return bytes_readed;
}

/*!
 * Clears a queue transmitting or receiving of port \a nport
 * depending on a parameter \a dir.
 * \param nport Port number as sio_com_t.
 * \param dir Direction of the transmit/receive as sio_dir_t (you can install them on the OR).
 * \return -1 on error.
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

/*!
 * Returns number of bytes in the port \a nport available to read.
 * \param nport Port number as sio_com_t.
 * \return Number of bytes available for reading or 0 on error.
 */
int sio_rx_available(sio_com_t nport)
{
    return (uarts[nport]) ? (uarts[nport]->rx.chars) : (0);
}

/*!
 * Close a port \a nport.
 * \param nport Port number as sio_com_t.
 */
void sio_close(sio_com_t nport)
{
    if (!uarts[nport]) {
        return;
    }

    int is_block_mode = (F_BLOCK_MODE & uarts[nport]->flags);

    // Wait end of transfer.
    if (is_block_mode) {
        while (uarts[nport]->tx.chars > 0) {}
        while (!(inpw(uarts[SIO_COM_PGM]->addr.lsr) & SIO_LSR_ETHR)) {}
    }

    ///sio_clear(nport, SIO_TX_DIRECTION | SIO_RX_DIRECTION);/// ???

#ifdef COM_PGM // PGM close.
    if (SIO_COM_PGM == nport) {
        _disable();
        outpw(0xFF28, (inpw(0xFF28) | intmasks[SIO_COM_PGM])); // Disable UART interrupt.
        // Interrupts.
        _dos_setvect(intnums[SIO_COM_PGM], old_vec_pgm); // Restore old interrupt vector.
        // Restore old configuration.
        outpw(uarts[SIO_COM_PGM]->addr.ier, old_ier); // Interrupt enable
        outpw(uarts[SIO_COM_PGM]->addr.mcr, old_brd); // Restore old baud rate.
        outpw(uarts[SIO_COM_PGM]->addr.lcr, old_lcr); // Restore old configuration.
        outpw(0xFF22, 0x0014); //Reset UART interrupt.
        outpw(0xFF28, inpw(0xFF28) & (~intmasks[SIO_COM_PGM])); // Enable UART interrupt.
        _enable();
    } else {
#endif
        // Disable UART interrupt.
        outp(uarts[nport]->addr.ier, 0x00);
        // Disable FIFO.
        outp(uarts[nport]->addr.iir_fcr, (inpw(uarts[nport]->addr.iir_fcr) & (~SIO_FCR_EF)));

        // COM1 and/or COM4
        if (((SIO_COM1 == nport) && (!uarts[SIO_COM4]))
                || ((SIO_COM4 == nport) && (!uarts[SIO_COM1]))) {// COM1 open and COM4 not open,
                                                                 // COM4 open and COM1 not open?

            _disable();
            outpw(0xFF28, (inpw(0xFF28) | intmasks[nport])); // Disable external UART interrupt.
            _enable();

            if (old_vecs[0]) {
                _dos_setvect(intnums[nport], old_vecs[0]); // Restore old interrupt vector.
            }
        }

        // COM2
        if (SIO_COM2 == nport) {

            _disable();
            outpw(0xFF28, (inpw(0xFF28) | intmasks[nport])); // Disable external UART interrupt.
            _enable();

            if (old_vecs[1]) {
                _dos_setvect(intnums[nport], old_vecs[1]); // Restore old interrupt vector.
            }
        }
#ifdef COM_PGM
    }
#endif

    free(uarts[nport]->tx.data);
    free(uarts[nport]->rx.data);
    free(uarts[nport]);
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
