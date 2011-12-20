/*********************************************************************************************
Project :
Version :
Date    : 30.03.2011
Author  : Denis Shienkov
Company :
Comments: A library for work with sockets in your controllers ADAM 5000 series.
License : New BSD
**********************************************************************************************/

/*! \file nio.h
 *
 * Abbreviation of the module (file) "nio" - Network Input Output.
 *
 * This is the header file for the module implementation "nio.cpp".
 * This header file is declared interface to access and work with the network
 * PLC ADAM 5510/TCP, and declared the corresponding data types.
 * 
 * \note This code is selected and adapted from the code to work 
 * with sockets from Datalight Sockets.
 */

#ifndef NIO_H
#define NIO_H

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! 
 * Codes a functions (AH) caused to be used sockets.
 */
typedef enum FUNCTION_NUMBERS {
    GET_BUSY_FLAG              = 0x0100,
    GET_KERNEL_INFO            = 0x0200,
    GET_ADDRESS                = 0x0500,
    GET_NET_INFO               = 0x0600,
    CONVERT_DC_SOCKET          = 0x0700,
    RELEASE_SOCKET             = 0x0800,
    RELEASE_DC_SOCKETS         = 0x0900,
    IS_SOCKET                  = 0x0D00,
    SELECT_SOCKET              = 0x0E00,
    GET_NET_VERSION            = 0x0F00,
    SHUT_DOWN_NET              = 0x1000,
    DISABLE_ASYNC_NOTIFICATION = 0x1100,
    ENABLE_ASYNC_NOTIFICATION  = 0x1200,
    CONNECT_SOCKET             = 0x1300,
    GET_PEER_ADDRESS           = 0x1600,
    EOF_SOCKET                 = 0x1800,
    ABORT_SOCKET               = 0x1900,
    WRITE_SOCKET               = 0x1A00,
    READ_SOCKET                = 0x1B00,
    WRITE_TO_SOCKET            = 0x1C00,
    READ_FROM_SOCKET           = 0x1D00,
    FLUSH_SOCKET               = 0x1E00,
    SET_ASYNC_NOTIFICATION     = 0x1F00,
    SET_OPTION                 = 0x2000,
    GET_DC_SOCKET              = 0x2200,
    LISTEN_SOCKET              = 0x2300,
    ABORT_DC_SOCKETS           = 0x2400,
    GET_SOCKET                 = 0x2900,
    GET_KERNEL_CONFIG          = 0x2A00,
    SET_ALARM                  = 0x2B00,
    ICMP_PING                  = 0x3000,
    PARSE_ADDRESS              = 0x5000,
    LOOKUP_HOST_TABLE          = 0x5100,
    RESOLVE_NAME               = 0x5400,
    JOIN_GROUP                 = 0x6000,
    LEAVE_GROUP                = 0x6100,
    IFACE_IOCTL                = 0x6200,
    GET_SOCKETS_VERSION        = 0x6300
} function_numbers_t;

/*! 
 * IOCTL functions codes.
 */
typedef enum IOCTL_FUNCTIONS {
    IOCTL_CONNECT     = 0, /*!< Start dial on ASY. */
    IOCTL_DISCONNECT  = 1, /*!< Drop modem on ASY. */
    IOCTL_ENABLEPORT  = 2, /*!< Enable port on ASY. */
    IOCTL_DISABLEPORT = 3, /*!< Disable port on ASY. */
    IOCTL_ENABLEDOD   = 4, /*!< Enable-dial-on-demand. */
    IOCTL_DISABLEDOD  = 5, /*!< Disable dial-on-demand. */
    IOCTL_GETSTATUS   = 6  /*!< Get dial status. */
} ioctl_functions_t;

/*! 
 * Status bits returned on IOCTL_GETSTATUS.
 */
typedef enum STATUS_BITS {
    ST_DTR            = 0x0001, /*!< Modem Data Terminal Ready. */
    ST_RTS            = 0x0002, /*!< Request To Send. */
    ST_CTS            = 0x0004, /*!< Clear To Send. */
    ST_DSR            = 0x0008, /*!< Data Set Ready. */
    ST_RI             = 0x0010, /*!< Ring Indicator. */
    ST_DCD            = 0x0020, /*!< Data Carrier Detect. */
    ST_CONNECTED      = 0x0040, /*!< Modem is connected. */
    ST_MODEMSTATE     = 0x0700, /*!< Modem state mask. */
    STM_NONE          = 0x0000, /*!< No modem on por.t */
    STM_IDLE          = 0x0100, /*!< Modem is idle. */
    STM_INITIALIZING  = 0x0200, /*!< Modem is initializing. */
    STM_DIALING       = 0x0300, /*!< Modem is dialing. */
    STM_CONNECTING    = 0x0400, /*!< Modem is connecting. */
    STM_ANSWERING     = 0x0500, /*!< Modem is answering. */
    STPPPP_IN         = 0x0800, /*!< PPP incoming call. */
    STPPP_STATE       = 0x7000, /*!< PPP state. */
    STPPP_DEAD        = 0x0000, /*!< PPP dead. */
    STPPP_LCP         = 0x1000, /*!< PPP LCP state. */
    STPPP_AP          = 0x2000, /*!< PPP Authentication state. */
    STPPP_READY       = 0x3000, /*!< PPP Ready (IPCP state). */
    STPPP_TERMINATING = 0x4000  /*!< PPP Terminating. */
} status_bits_t;

/*! 
 * If an error occurs, the Carry Flag is set and an error
 * code is returned in the AX register.
 */
typedef enum NET_ERRORS {
    NO_ERR             = 0,  /*!< No error. */
    ERR_IN_USE         = 1,  /*!< A connection alreadyexists. */
    ERR_DOS            = 2,  /*!< A DOS error occured. */
    ERR_NO_MEM         = 3,  /*!< No memory to perform function. */
    ERR_NOT_NET_CONN   = 4,  /*!< Connection does not exist. */
    ERR_ILLEGAL_OP     = 5,  /*!< Protocol or mode not supported. */
    ERR_NO_HOST        = 7,  /*!< No host address specified. */
    ERR_TIMEOUT        = 13, /*!< The function timed out. */
    ERR_HOST_UNKNOWN   = 14, /*!< Unknown host has been specified. */
    ERR_BAD_ARG        = 18, /*!< Bad arguments. */
    ERR_EOF            = 19, /*!< Connection closed by peer. */
    ERR_RESET          = 20, /*!< Connection reset by peer. */
    ERR_WOULD_BLOCK    = 21, /*!< Operation would block. */
    ERR_UNBOUND        = 22, /*!< Descriptor not yet assigned. */
    ERR_NO_SOCKET      = 23, /*!< No descriptor is available. */
    ERR_BAD_SYS_CALL   = 24, /*!< Bad parameter in call. */
    ERR_NOT_ESTAB      = 26, /*!< Connection not yet established. */
    ERR_RE_ENTRY       = 27, /*!< Kernel use, try again later. */
    ERR_TERMINATING    = 29, /*!< Kernel unloading. */
    ERR_API_NOT_LOADED = 50  /*!< API is not loaded. */
} net_errors_t;

/*!
 * Connection types for nio_connect() 
 * and nio_listen().
 */
typedef enum CONNECTION_TYPES {
    DATA_GRAM = 3,
    STREAM    = 4
} connection_types_t;

/*! 
 * Network address structure for NetConnect() 
 * and NetListen().
 */
typedef struct NET_ADDR {
    u32 remote_host; /*!< IP address of remote host */
    u16 remote_port; /*!< Remote port address */
    u16 local_port;  /*!< Local port address */
    u8 protocol;     /*!< Protocol */
} net_addr_t;

/*! 
 * Flags for nio_recv(), nio_recv_from(),
 * nio_send(), nio_send_to().
 */
typedef enum NET_FLAGS {
    NET_FLG_OOB          = 0x0001, /*!< Out of band data. */
    NET_FLG_PEEK         = 0x0002, /*!< Don't dequeue data. */
    NET_FLG_PUSH         = 0x0010, /*!< Disregard Nagle heuristic. */
    NET_FLG_NON_BLOCKING = 0x0040, /*!< Don't block. */
    NET_FLG_BROADCAST    = 0x0080, /*!< Broadcast data. */
    NET_FLG_MC_NOECHO    = 0x1000  /*!< Don't echo multicast. */
} net_flags_t;

/*! 
 * Values used in nio_set_opt().
 */
typedef enum NET_OPTIONS {
    NET_OPT_NON_BLOCKING = 0x01, /*!< Don't block on system calls. */
    NET_OPT_TIMEOUT      = 0x02, /*!< Timeout system calls. */
    NET_OPT_WAIT_FLUSH   = 0x0C  /*!< Honor NetFlgPush on socket. */
} net_options_t;

/*! 
 * Values for async notification routines.
 */
typedef enum NET_ASYNC_NOTIFY_ROUTE {
    NET_AS_ALARM     = 0x00,
    NET_AS_OPEN      = 0x01,
    NET_AS_RCV       = 0x02,
    NET_AS_XMT       = 0x03,
    NET_AS_XMT_FLUSH = 0x04,
    NET_AS_FCLOSE    = 0x05,
    NET_AS_CLOSE     = 0x06,
    NET_AS_ERROR     = 0x07,

    MAX_AS_EVENT     = NET_AS_ERROR
} net_async_notify_route_t;

/*! 
 * Network info structure.
 */
typedef struct NET_INFO {
    int nclass;
    int ntype;
    int nnum;
    u32 ipaddr;
    u32 ipsubnet;
    int up;
    u32 in;
    u32 out;         /*!< # of packets transmitted. */
    u32 inerr;       /*!< # of receiver errors. */
    u32 outerr;      /*!< # of transmitter errors. */
    int lanlen;      /*!< length of local net address. */
    u8 far *lanaddr; /*!< pointer to the lan address. */
} net_info_t;

/*! 
 * Kernel configuration structure.
 */
typedef struct KERNEL_CONFIG {
    u8 maxtcp;     /*!< # of TCP connections allowed. */
    u8 maxudp;     /*!< # of UDP connections allowed. */
    u8 maxip;      /*!< # of IP connections allowed. */
    u8 maxraw;     /*!< # of RAW_NET connections allowed. */
    u8 acttcp;     /*!< # of TCP connections in use. */
    u8 actudp;     /*!< # of UDP connections in use. */
    u8 actip;      /*!< # of IP connections in use. */
    u8 actraw;     /*!< # of RAW_NET connections in use. */
    u16 actdcs;    /*!< # of active Dos Compatible Sockets. */
    u16 actsoc;    /*!< # of active Sockets. */
    u8 maxh;       /*!< Maximum header on an attached network. */
    u8 maxt;       /*!< Maximum trailer on an attached network. */
    u16 bufsize;   /*!< Size of a large packet buffer. */
    u16 netnum;    /*!< Number of network interfaces attached. */
    u32 ticks;     /*!< Milliseconds since kernel started. */
    u32 broadcast; /*!< IP broadcast address in use. */
} kernel_config_t;

/*! 
 * Kernel info codes.
 */
typedef enum KERNEL_INFO {
    K_INF_TCP_CONS    = 0x0400, /*!< Gets number of Sockets (DC + normal). */
    K_INF_HOST_TABLE  = 0x0800, /*!< Gets name of file containing host table. */
    K_INF_DNS_SERVERS = 0x0900, /*!< Gets IP addresses of DNS Servers. */
    K_INF_HOSTNAME    = 0x5A00,
    K_INF_MAC_ADDR    = 0x8100,
    K_INF_BCAST_ADDR  = 0x8400, /*!< Gets broadcast IP address. */
    K_INF_IP_ADDR     = 0x8200, /*!< Gets IP address of first interface. */
    K_INF_SUBNET_MASK = 0x8500  /*!< Gets netmask of first interface. */
} kernel_info_t;

/*! 
 * Group address structure for nio_join_group() 
 * and nio_leave_group().
 */
typedef struct GROUP_ADDR {
    u32 groupaddr; /*!< Group address. */
    u32 ifaceaddr; /*!< IP address of interface to use, 0 for default. */
} group_addr_t;

/*!
 * The following globals are set after each call into sockets
 * and indicate particular error codes upon failure of a
 * function.
 */
extern int nioerrno;
extern int niosuberrno;

// Globals Sockets API
int nio_disable_async_notify(void);
int nio_enable_async_notify(void);
u32 nio_address(int socket);
int nio_peer_address(int socket, net_addr_t *na);
int nio_kernel_info(int reserved, kernel_info_t code, u8 devid, void *data, u16 *size);
int nio_version(void);
int nio_icmp_ping(u32 host, int length);
int nio_is_socket(int socket);
int nio_dcsocket(void);
int nio_socket(void);
int nio_kernel_cfg(kernel_config_t *kc);
int nio_convert_dcsocket(int socket);
int nio_info(int socket, net_info_t *ni);
int nio_connect(int socket, connection_types_t ntype, net_addr_t *na);
int nio_listen(int socket, connection_types_t ntype, net_addr_t *na);
int nio_select(int maxid, long *iflags, long *oflags);
int nio_recv(int socket, char *buf, u16 len, net_addr_t *na, u16 flags);
int nio_recv_from(int socket, char *buf, u16 len, net_addr_t *na, u16 flags);
int nio_send(int socket, char *buf, u16 len, u16 flags);
int nio_send_to(int socket, char *buf, u16 len, net_addr_t *na, u16 flags);
int nio_eof(int socket);
int nio_flush(int socket);
int nio_release(int socket);
int nio_dcrelease(void);
int nio_abort(int socket);
int nio_dcabort(void);
int nio_shutdown(void);
int nio_set_alarm(int socket, u32 time, int (far *handler)(), u32 hint);
int far *nio_set_async_notify(int socket, int event, int (far *handler)(), u32 hint);
int far nio_async_notify_handler(void);
u32 nio_resolve_name(char *zname, char *cname, int cnamelen);
u32 nio_parse_address(char *zname);
int nio_set_opt(int socket, int level, int opt, u32 optval, int len);
int nio_join_group(u32 groupaddr, u32 ifaceaddr);
int nio_leave_group(u32 groupaddr, u32 ifaceaddr);
int nio_ioctl(char *zname, ioctl_functions_t fun);
int nio_sockets_version(void);

#ifdef __cplusplus
}
#endif
#endif // NIO_H
