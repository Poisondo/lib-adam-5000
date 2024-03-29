/*********************************************************************************************
Project :
Version :
Date    : 24.03.2011
Author  : Oleg Diyakonov, Denis Shienkov
Company :
Comments: A library for work with serial ports in your controllers ADAM 5000 series.
License : New BSD
**********************************************************************************************/

/*! \file sio.h
 *
 * Abbreviation of the module (file) "sio" - Serial Input Output.
 *
 * This is the header file for the module implementation "sio.cpp".
 * This header file is declared interface for accessing and working with serial
 * ports COM1 - COM4 PLC ADAM 5510, and declared to the appropriate data types.
 */

#ifndef SIO_H
#define SIO_H

#include "platformdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Enable programming port COM3.
 */
#define COM_PGM

/*!
 * Possible (available) serial ports on PLC.
 */
typedef enum SIO_COM {

    SIO_COM1    = 0, /*!< RS-232 COM1. */
    SIO_COM2    = 1, /*!< RS-485 COM2. */

#ifdef COM_PGM
    SIO_COM_PGM = 2, /*!< RS232 COM3 - for programming the PLC (terminal). */
#endif

    SIO_COM4    = 3  /*!< RS-232/485 COM4. */

} sio_com_t;

/*!
 * Data transfer directions.
 */
typedef enum SIO_DIR {
    SIO_RX_DIRECTION = 0x01, /*!< Input direction. */
    SIO_TX_DIRECTION = 0x02  /*!< Output direction. */
} sio_dir_t;

/*!
 * Error codes when working with the ports.
 *
 * These error codes returned by the module functions
 * in the shared variable \a sioerrno.
 */
typedef enum SIO_ERR {
    SIO_ERR_NONE                = 0, /*!< No errors. */
    SIO_ERR_INVALID_PORT_NUM    = 1, /*!< Port number is not supported. */
    SIO_ERR_NO_UART             = 2, /*!< Not detected UART chip. */
    SIO_ERR_PORT_ALREADY_OPEN   = 3, /*!< Port is already open. */
    SIO_ERR_PORT_NOT_OPEN       = 4, /*!< Port is not open. */
    SIO_ERR_INVALID_BUFFER_SIZE = 5, /*!< Unsupported buffer size. */
    SIO_ERR_ILLEGAL_SETTING     = 6, /*!< Incorrect configuration parameters. */
    SIO_ERR_UART_NOT_SUPPORTED  = 7, /*!< This type of UART chip is not supported. */
    SIO_ERR_NOT_MEMORY          = 8  /*!< No memory to create buffers, etc. */
} sio_err_t;

/*!
 * Supported baud rates.
 */
typedef enum SIO_SPEED {
    SIO_BPS_50     = 2304, /*!< 50 baud. */
    SIO_BPS_300    = 384,  /*!< 300 baud. */
    SIO_BPS_600    = 192,  /*!< 600 baud. */
    SIO_BPS_2400   = 48,   /*!< 2400 baud. */
    SIO_BPS_4800   = 24,   /*!< 4800 baud. */
    SIO_BPS_9600   = 12,   /*!< 9600 baud. */
    SIO_BPS_19200  = 6,    /*!< 19200 baud. */
    SIO_BPS_38400  = 3,    /*!< 38400 baud. */
    SIO_BPS_57600  = 2,    /*!< 57600 baud. */
    SIO_BPS_115200 = 1     /*!< 115200 baud. */
} sio_speed_t;

/*!
 * Supported number of data bits.
 */
typedef enum SIO_DATABITS {
    SIO_DATA5 = 0x00, /*!< 5 bit. */
    SIO_DATA6,        /*!< 6 bit. */
    SIO_DATA7,        /*!< 7 bit. */
    SIO_DATA8         /*!< 8 bit. */
} sio_databits_t;

/*!
 * Supported types of parity.
 */
typedef enum SIO_PARITY {
    SIO_PAR_NONE = 0x00, /*!< No parity. */
    SIO_PAR_EVEN = 0x18, /*!< Even. */
    SIO_PAR_ODD  = 0x08, /*!< Odd. */
    SIO_PAR_ZERO = 0x38, /*!< Space. */
    SIO_PAR_ONE  = 0x28  /*!< Marker. */
} sio_parity_t;

/*!
 * Supported number of stop bits.
 */
typedef enum SIO_STOPBITS {
    SIO_STOP1 = 0x00, /*!< One stop bit. */
    SIO_STOP2 = 0x04  /*!< Two stop bits. */
} sio_stopbits_t;

/*!
 * Supported open modes.
 */
typedef enum SIO_MODE {
    SIO_BLOCK_MODE = 0, /*!< Blocking mode. */
    SIO_UNBLOCK_MODE    /*!< Non-blocking mode. */
} sio_mode_t;

/*!
 * Error code.
 *
 * In this variable function of the module record last
 * error code on failure. User must use this variable
 * only for r/o operations.
 */
extern int sioerrno;

int sio_open(sio_com_t nport, sio_mode_t mode, int tx_buf_size, int rx_buf_size);
int sio_configure(sio_com_t nport, sio_speed_t baud, sio_parity_t parity, sio_databits_t databits, sio_stopbits_t stopbits);
int sio_send(sio_com_t nport, const char *buf, int len);
int sio_recv(sio_com_t nport, char *buf, int len);
int sio_clear(sio_com_t nport, sio_dir_t dir);
int sio_rx_available(sio_com_t nport);
void sio_close(sio_com_t nport);

#ifdef __cplusplus
}
#endif
#endif // SIO_H
