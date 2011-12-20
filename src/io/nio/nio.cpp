/*********************************************************************************************
Project :
Version :
Date    : 30.03.2011
Author  : Denis Shienkov
Company :
Comments: A library for work with sockets in your controllers ADAM 5000 series.
License : New BSD
**********************************************************************************************/

/*! \file nio.cpp
 *
 * Abbreviation of the module (file) "nio" - Network Input Output.
 *
 * This module implements an interface to access and work with sockets PLC ADAM 5510/TCP.
 *
 * \note This code is selected and adapted from the code to work 
 * with sockets from Datalight Sockets.
 */

#include <dos.h>
#include "nio.h"


//--------------------------------------------------------------------------------------------------------//
/*** Private variables ***/

/*!
 */
enum {
    HANDLER_STACK_SIZE    = 500, /*!< The stack size for asynchronous callbacks. */
    SOCKETS_API_INTERRUPT = 0x61 /*!< The following global is used to tell call_sock_dos_api()
                                      functions as an interrupt stack sockets connections.
                                      The default is 0x61, unless overridden on the command line
                                      load or SOCKETM SOCKETSP. */
};

/*!
 * Sockets interface driver implemented through a software interrupt with
 * set of appropriate registers a function that
 * various parameters, passing a pointer to the data.
 */
typedef struct X86REGS {
    u16 ax; u16 bx; u16 cx; u16 dx;
    u16 si; u16 di; u16 ds; u16 es;
} x86regs_t;

//--------------------------------------------------------------------------------------------------------//
/*** Private functions ***/

/*! 
 * Causes a kernel sockets with a software interrupt SOCKETS_API_INTERRUPT
 * with the registers defined in x86regs_t structure.
 * \param x86r pointer to a structure which has x86regs_t field
 * register for the call sockets interrupts. This framework updates
 * the new value of the register after the interrupt Sockets.
 * \return On success AX returns sockets call; otherwise -1 if not found Sockets API.
 */
static int call_sock_dos_api(x86regs_t *x86r)
{
    union REGS r;
    struct SREGS s;
    nioerrno = niosuberrno = NO_ERR;
    /* Make certain the sockets interrupt is set */
    //if (!SOCKETS_API_INTERRUPT) {
    //    nioerrno = ERR_API_NOT_LOADED;
    //    return -1;
    //}
    // The call interrupts the respective socket set registers for Sockets API.
    r.x.ax = x86r->ax;
    r.x.bx = x86r->bx;
    r.x.cx = x86r->cx;
    r.x.dx = x86r->dx;
    r.x.si = x86r->si;
    r.x.di = x86r->di;
    s.ds = x86r->ds;
    s.es = x86r->es;
    int86x(SOCKETS_API_INTERRUPT, &r, &r, &s);
    x86r->ax = r.x.ax;
    x86r->bx = r.x.bx;
    x86r->cx = r.x.cx;
    x86r->dx = r.x.dx;
    x86r->si = r.x.si;
    x86r->di = r.x.di;
    x86r->ds = s.ds;
    x86r->es = s.es;
    
    // Any errors reported to a set of flags, and network errors in the lower 
    // 8 bits and any sub-error codes in the upper 8 bits. 
    if (0x01 & r.x.cflag) {
        nioerrno = x86r->ax & 0x00FF;
        niosuberrno = x86r->ax >> 8;
        return -1;
    }
    return x86r->ax;
}

// Template handler function that calls the right place.
typedef int (far *FH)(int, int, u32);

//--------------------------------------------------------------------------------------------------------//
/*** Public variables ***/

/*!
 */
int nioerrno = NO_ERR;
int niosuberrno = NO_ERR;

//--------------------------------------------------------------------------------------------------------//
/*** Public functions ***/

/*! 
 * \return On success returns the version number of the Sockets API;
 * otherwise -1 if the number of API is not found;
 */
int nio_sockets_version(void)
{
    x86regs_t r;
    r.ax = GET_SOCKETS_VERSION;
    return call_sock_dos_api(&r);
}

/*! 
 * Turns off asynchronous notifications (callbacks).
 * \return On success returns 0 if disabled, 1 if enabled;
 * otherwise -1 and set error code to nioerrno and niosuberrno.
 */
int nio_disable_async_notify(void)
{
    x86regs_t r;
    r.ax = DISABLE_ASYNC_NOTIFICATION;
    return call_sock_dos_api(&r);
}

/*! 
 * Turns on asynchronous notifications (callbacks).
 * \return On success returns previous status of the notification;
 * otherwise -1 and set error code to nioerrno and niosuberrno.
 */
int nio_enable_async_notify(void)
{
    x86regs_t r;
    r.ax = ENABLE_ASYNC_NOTIFICATION;
    return call_sock_dos_api(&r);
}

/*! 
 * Checks DOS-compatible \a socket.
 * \param socket Socket descriptor.
 * \return On success 0; otherwise -1 and set error code to 
 * nioerrno and niosuberrno.
 */
int nio_is_socket(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = IS_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Gets the DOS-compatible socket descriptor. This function 
 * calls the DOS, DOS to open a handle to the file.
 * \return On success number of socket descriptor;
 * otherwise -1 and set error code to nioerrno.
 */
int nio_dcsocket(void)
{
    x86regs_t r;
    r.ax = GET_DC_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Get socket descriptor.
 * \return On success number of socket descriptor;
 * otherwise -1 and set error code to nioerrno.
 */
int nio_socket(void)
{
    x86regs_t r;
    r.ax = GET_SOCKET;
    return call_sock_dos_api(&r);
}

/*!
 * Returns the version number of compatible API.
 * \return On success 0x0214; otherwise -1
 * and set error code to nioerrno.
 */
int nio_version(void)
{
    x86regs_t r;
    r.ax = GET_NET_VERSION;
    return call_sock_dos_api(&r);
}

/*!
 * Converts DOS compatible socket descriptor \a socket in a
 * normal socket descriptor. This function calls the DOS, to
 * close the DOS file descriptor.
 * \param socket DOS socket descriptor.
 * \return On success socket descriptor; otherwise -1
 * and set error code to nioerrno.
 */
int nio_convert_dcsocket(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = CONVERT_DC_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Closes the stream (TCP) connection (sends FIN). After calling 
 * nio_eof(), you can not call nio_send(). Socket, however, remains
 * open for reading until the feast closes the connection.
 * \param socket Socket descriptor.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_eof(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = EOF_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Clears (force sends) all output queued TCP connection.
 * \param socket Socket descriptor.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_flush(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = FLUSH_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Closes connection and frees all resources.
 * During the exchange of data (TCP) connection, this function must be called 
 * once the connection be closed from both sides, or (an awkward closing) 
 * can lead to discharge.
 * \param socket Socket descriptor.
 * \return On success socket descriptor; otherwise -1 and 
 * set error code to nioerrno.
 */
int nio_release(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = RELEASE_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Closes all connections and frees all resources associated with
 * DOS-compatible sockets.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_dcrelease(void)
{
    x86regs_t r;
    r.ax = RELEASE_DC_SOCKETS;
    return call_sock_dos_api(&r);
}

/*! 
 * Terminates the network connection and releases all resources.
 * This function is unpredictable close to stream discharge connection.
 * \param socket Socket descriptor.
 * \return On success socket descriptor; otherwise -1 and 
 * set error code to nioerrno.
 */
int nio_abort(int socket)
{
    x86regs_t r;
    r.ax = ABORT_SOCKET;
    r.bx = socket;
    return call_sock_dos_api(&r);
}

/*!
 * Aborts all DOS-compatible connections.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_dcabort(void)
{
    x86regs_t r;
    r.ax = ABORT_DC_SOCKETS;
    return call_sock_dos_api(&r);
}

/*! 
 * Shuts down the network and unloads Sockets TCP / IP kernel.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_shutdown(void)
{
    x86regs_t r;
    r.ax = SHUT_DOWN_NET;
    return call_sock_dos_api(&r);
}

/*! 
 * Returns the local address of the IP-connection.
 * In the case of a single host interface - an IP-address of the host.
 * If more than one interface IP-address of the interface used to route
 * traffic for certain connections.
 * \param socket Socket descriptor.
 * \return On success IP address; otherwise -1 and 
 * set error code to nioerrno.
 */
u32 nio_address(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = GET_ADDRESS;
    return (-1 == call_sock_dos_api(&r)) ? (0L) : (((u32)r.dx << 16L) + r.ax);
}

/*! 
 * Returns network address information \a na feast on the address 
 * connected \a socket.
 * \param socket Socket descriptor.
 * \param na The structure information as net_addr_t which is 
 * returned address of the connected peer.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_peer_address(int socket, net_addr_t *na)
{
    x86regs_t r;
    r.ds = FP_SEG(na);
    r.dx = FP_OFF(na);
    r.bx = socket;
    r.ax = GET_PEER_ADDRESS;
    return call_sock_dos_api(&r);
}

/*! 
 * Returns specific information from the kernel.
 * \param reserved Reserved and not used, must be set to 0.
 * \param code Code the information requested in the kernel 
 * as kernel_info_t.
 * \param devid ???.
 * \param data A pointer to a data area in which information is
 * returned kernel.
 * \param size Pointer to the data area into which return the size 
 * of the kernel information.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_kernel_info(int reserved, kernel_info_t code, u8 devid, void *data, u16 *size)
{
    x86regs_t r;
    // Pointers in this module may be distant or not, depending on 
    // the memory model used by the compiler. The following code will
    // ensure that long pointer is passed to the Sockets completely null
    // if the offset pointer is zero.
    r.si = FP_OFF(data);
    r.ds = (data) ? (FP_SEG(data)) : (0);
    r.di = FP_OFF(size);
    r.es = (size) ? (FP_SEG(size)) : (0);
    r.bx = reserved;
    r.dx = code + devid;
    r.ax = GET_KERNEL_INFO;
    return call_sock_dos_api(&r);
}

/*! 
 * Ping sends ICMP (echo request) and waits until a response
 * or within six seconds, if no response is received.
 * \note Function nio_icmp_ping() always blocked!
 * \param host IP-address of the host to ping.
 * \param length Number of bytes of data in the request for the ping.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_icmp_ping(u32 host, int length)
{
    x86regs_t r;
    r.bx = (u16)(host & 0xFFFFL);
    r.dx = (u16)(host >> 16);
    r.cx = length;
    r.ax = ICMP_PING;
    return call_sock_dos_api(&r);
}

/*!
 * Returns a \a kc kernel configuration.
 * \param kc Pointer to the structure of the kernel config as kernel_config_t.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_kernel_cfg(kernel_config_t *kc)
{
    x86regs_t r;
    r.ds = FP_SEG(kc);
    r.si = FP_OFF(kc);
    r.ax = GET_KERNEL_CONFIG;
    return call_sock_dos_api(&r);
}

/*! 
 * Returns a network information \a ni.
 * \param socket Socket descriptor.
 * \param ni Pointer to a structure as net_info_t.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_info(int socket, net_info_t *ni)
{
    x86regs_t r;
    r.ds = FP_SEG(ni);
    r.si = FP_OFF(ni);
    r.bx = socket;
    r.ax = GET_NET_INFO;
    return call_sock_dos_api(&r);
}

/*! 
 * Creates a network connection.
 * If the \a socket is specified as -1, DOS-compatible connector is 
 * assigned. In this case, only the DOS opens a file descriptor.
 * If the \a socket is specified as a non-blocking, or \a ntype is 
 * listed as a type of communication that DATAGRAM then this call returns
 * immediately. In the case of compound STREAM link may not yet be 
 * installed and nio_recv() can be used to test the connection. While 
 * nio_recv() returns ERR_NOT_ESTAB connection is not established,
 * good return or return an error when ERR_WOULD_BLOCK indicates that the
 * connection is established. A more sophisticated method uses 
 * nio_set_async_notify() with NET_AS_OPEN to test the connection.
 * NET_AS_ERROR also be set for notification of a failed attempt to open it.
 * \param socket Socket descriptor.
 * \param ntype Connection type as connection_types_t.
 * \param na Pointer to a structure as net_addr_t.
 * \return On success socket descriptor; otherwise -1 and set error 
 * code to nioerrno.
 */
int nio_connect(int socket, connection_types_t ntype, net_addr_t *na)
{
    x86regs_t r;
    na->protocol = 6;
    r.ds = FP_SEG(na);
    r.si = FP_OFF(na);
    r.bx = socket;
    r.dx = ntype;
    r.ax = CONNECT_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Listens for network connections.
 * If the socket is specified as -1 then assigned a DOS-compatible socket. In this case, 
 * only the DOS opens a file descriptor.
 * If the \a socket is specified as non-blocking, or type \a ntype listed 
 * as DATAGRAM then this call returns immediately. In the case of compounds such 
 * as STREAM connection may not yet be established. Function nio_recv() can be used 
 * to test the connection. While nio_recv() returns ERR_NOT_ESTAB, is not connected.
 * The success or failure return ERR_WOULD_BLOCK indicates that the connection
 * is established. A more sophisticated method uses nio_set_async_notify() with 
 * NET_AS_OPEN to test the connection. NET_AS_ERROR also be set for notification 
 * of a failed attempt to open it.
 * \param socket Socket descriptor.
 * \param ntype Connection type as connection_types_t.
 * \param na Pointer to a structure as net_addr_t.
 * \return On success socket descriptor; otherwise -1 and set error code to nioerrno.
 */
int nio_listen(int socket, connection_types_t ntype, net_addr_t *na)
{
    x86regs_t r;
    na->protocol = 6;
    r.ds = FP_SEG(na);
    r.si = FP_OFF(na);
    r.bx = socket;
    r.dx = ntype;
    r.ax = LISTEN_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Check all DOS-compatible sockets for data availability for read and write.
 * 32-bit \a iflags are 32 DC socket is filled for each socket to receive data
 * and the other 32-bit \ a oflags DC socket ready for writing.
 * The least significant bit represents socket with a value of 0 and the most 
 * significant bit represents the value of the socket 31.
 * Bits of unused outlets, remain unchanged. Output variables \a iflags and
 * \a oflags filled with current values ​​of status socket.
 * \param maxid Number of scanned sockets.
 * \param iflags Pointer to the input flags indicating the readiness to receive data.
 * \param oflags Pointer to the output flags indicating the willingness to transfer data.
 * \return On success 0; otherwise -1 and set error code to nioerrno.
 */
int nio_select(int maxid, long *iflags, long *oflags)
{
    x86regs_t r;
    r.ds = FP_SEG(iflags);
    r.dx = FP_OFF(iflags);
    r.es = FP_SEG(oflags);
    r.di = FP_OFF(oflags);
    r.bx = maxid;
    r.ax = SELECT_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Reads from the network using sockets.
 * Returned as soon as any non-zero amount of data available, regardless of 
 * the blocking state. If the operation is not blocked (when used nio_set_opt() with
 * the option NET_OPT_NON_BLOCKING or the flag with NET_FLG_NON_BLOCKING), nio_recv() 
 * returns immediately count of available data or error ERR_WOULD_BLOCK.
 * In the case of STREAM (TCP) socket, there is no record boundaries, and any amount
 * of data can be read at any time, regardless of the route by which he was sent to peers.
 * No data is truncated or lost, even if more data than the buffer size is available.
 * What has not returned one call back for the next call. If NULL buffer is specified, 
 * the number of bytes in the queue are returned. In the case of DATAGRAM (UDP) socket, 
 * the datagrams are returned in one call, if the buffer is too small and in this case
 * the data is truncated, thus maintaining record boundaries. The truncated data will be
 * lost. If data are available and both NET_FLG_PEEK NET_FLG_NON_BLOCKING and flags are 
 * not specified, the number of datagrams in turn receive a return. If data is available
 * and installed NET_FLG_PEEK? And NULL buffer is specified, the number of bytes in the 
 * next datagram is returned.
 * \param socket Socket descriptor.
 * \param buf A pointer to a buffer in which data will be read.
 * \param len Buffer size, ie the maximum number of bytes to read.
 * \param na Pointer to a structure net_addr_t receiving address information on local 
 * and remote port and remote IP-address.
 * \param flags Flags governing the operation (any combination of NET_FLG_PEEK and 
 * NET_FLG_NON_BLOCKING).
 * \return On success the number of bytes read; otherwise -1 and set error code to  nioerrno,
 * the return code of 0 indicates that the peer closes the connection.
 */
int nio_recv(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    // Pointers in this module may be far or not, depending on the memory model used by
    // the compiler. The following code will ensure that long pointer is passed
    // to the Sockets completely null if the offset pointer is zero.
    r.di = FP_OFF(na);
    r.es = (na) ? (FP_SEG(na)) : (0);
    r.si = FP_OFF(buf);
    r.ds = (buf) ? (FP_SEG(buf)) : (0);
    r.dx = flags;
    r.cx = len;
    r.bx = socket;
    r.ax = READ_SOCKET;
    return (-1 == call_sock_dos_api(&r)) ? (-1) : (r.cx);
}

/*! 
 * Reads from the network using sockets and is designed for use only for DATAGRAM sockets.
 * All datagrams with IP-addresses and ports return the appropriate values ​​in the structure 
 * net_addr_t while others are discarded. A value of zero for remote_host used as a 
 * template to get from any host and zero for remote_port used as a template to get out
 * of any port. The local port, local_port, can not be specified as zero. In other 
 * respects nio_recv_from() behaves the same way as nio_recv().
 * \note Please note the following anomaly: If the lock is disabled, failure with error
 * code ERR_WOULD_BLOCK perfectly normal and just means that no data at present.
 * \param socket Socket descriptor.
 * \param buf A pointer to a buffer in which data will be read.
 * \param len Buffer size, ie the maximum number of bytes to read.
 * \param na Pointer to a structure net_addr_t receiving address information on local 
 * and remote port and remote IP-address.
 * \param flags Flags governing the operation (any combination of NET_FLG_PEEK and NET_FLG_NON_BLOCKING).
 * \return On success the number of bytes read; otherwise -1 and set error code to nioerrno, 
 * the return code of 0 indicates that the peer has closed the connection.
 */
int nio_recv_from(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    // Pointers in this module may be far or not, depending on the memory model used by
    // the compiler. The following code will ensure that long pointer is passed
    // to the Sockets completely null if the offset pointer is zero.
    r.di = FP_OFF(na);
    r.es = (na) ? (FP_SEG(na)) : (0);
    r.si = FP_OFF(buf);
    r.ds = (buf) ? (FP_SEG(buf)) : (0);
    r.dx = flags;
    r.cx = len;
    r.bx = socket;
    r.ax = READ_FROM_SOCKET;
    return call_sock_dos_api(&r);
}

/*!
 * Writing to a network using sockets.
 * Number of bytes actually written in non-blocking socket, may be less than the length of \a len.
 * In this case, the transmission of unsent data must be repeated, preferably with some delay.
 * \param socket Socket descriptor.
 * \param buf A pointer to a buffer that contain the data for transmission.
 * \param len Buffer size, ie number of bytes to transfer.
 * \param flags Flags governing the operation (any combination of NET_FLG_OOB, NET_FLG_PUSH, 
 * NET_FLG_NON_BLOCKING, NET_FLG_BROADCAST, NET_FLG_MC_NOECHO).
 * \return On success the number of bytes transferred; otherwise -1, and set error 
 * code to nioerrno.
 */
int nio_send(int socket, char *buf, u16 len, u16 flags)
{
    x86regs_t r;
    r.ds = FP_SEG(buf);
    r.si = FP_OFF(buf);
    r.bx = socket;
    r.cx = len;
    r.dx = flags;
    r.ax = WRITE_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Writing to a network using UDP only.
 * \param socket Socket descriptor.
 * \param buf A pointer to a buffer that contain the data for transmission.
 * \param len Buffer size, ie number of bytes to transfer.
 * \param na Pointer to a structure net_addr_t, contains the local port to
 * send and the remote port and IP-address for the record.
 * \param flags Flags governing the operation (any combination of NET_FLG_NON_BLOCKING,
 * NET_FLG_BROADCAST).
 * \return On success the number of bytes transferred; otherwise -1, and set error 
 * code to nioerrno.
 */
int nio_send_to(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    r.ds = FP_SEG(buf);
    r.si = FP_OFF(buf);
    r.es = FP_SEG(na);
    r.di = FP_OFF(na);
    r.bx = socket;
    r.cx = len;
    r.dx = flags;
    r.ax = WRITE_TO_SOCKET;
    return call_sock_dos_api(&r);
}

/*! 
 * Sets the alarm timer.
 * \param socket Socket descriptor.
 * \param time Delay timer in milliseconds.
 * \param handler Far signaling address of the callback. See the description of
 * nio_set_async_notify() to format the callback function.
 * \param hint Argument passed to the callback function.
 * \return On success socket descriptor; otherwise -1 and set error code to nioerrno.
 */
int nio_set_alarm(int socket, u32 time, int (far *handler)(), u32 hint)
{
    x86regs_t r;
    r.ds = FP_SEG(handler);
    r.si = FP_OFF(handler);
    r.cx = (u16)(time >> 16);
    r.dx = (u16)(time & 0xFFFFL);
    r.es = (u16)(hint >> 16);
    r.di = (u16)(hint & 0xFFFFL);
    r.bx = socket;
    r.ax = SET_ALARM;
    return call_sock_dos_api(&r);
}

/*!
 * Sets the asynchronous notification (callback) for a specific event.
 * Other functions of CAPI can be named in the callback, except nio_resolve_name),
 * which can be called DOS. The callback is not compatible with the C argument to 
 * bypass convention and caution should be taken. Some manipulation of the CPU 
 * registers are required. This can be done by reference to the CPU registers, such
 * as _BX, or with assembler instructions. In the callback, the stack is supplied 
 * socket and can be very small, depending on the /C = command line parameter at 
 * boot Sockets. Stack segment is obviously not equal to the data segment, which can
 * cause problems with very small, small or medium memory model used.
 * The easiest way to solve this problem is to use a compact, large, or huge memory model.
 * Other options - the use of DS = SS compiler option or switch to make the stack data 
 * segment stack!.
 * If the callback function is written in C or C++, _loadds modifier can be used to set 
 * the data segment that module, which destroys the DS is used for the variable argument.
 * (That's why DS == SI entry for Sockets version 1.04 and above.)
 * An alternative method is to use the argument is nio_set_async_notify() in ES: DI, as
 * a pointer to a structure that is available with the basic code and callback.
 * If the DS is not installed on the data segment of the module, the functions not work: 
 * do not use them otherwise.
 * The callback is likely to be executed in interrupt mode with no guarantee of landing
 * for DOS. Do not use any functions, such as rispag() or E() in callback function, 
 * which can cause DOS to call. It is good programming practice to do as little as possible
 * to the callback. Set flags events that trigger operations in more stable times is 
 * recommended. Callback functions are not nesting. The callback function is not called 
 * while a callback is continuing, even if other functions are called api.
 * To mitigate the problem in paragraphs 2, 3 and 4 above, the handler is given in which api
 * uses a clue parameter to pass to address C-compatible processor, with a chimney, which is
 * also C-compatible. This handler is called nio_async_notify_handler().
 * The user handler named my_handler below, is called in the usual way with a stack of 500 bytes.
 * The stack size value can be set by changing the constant in HANDLER_STACK_SIZE api.
 * \param socket Socket descriptor.
 * \param event The event that can be installed from net_async_notify_route_t.
 * \param handler Far address of a callback function.
 * \param hint Argument passed to the callback function.
 * \return On success pointer to the previous handler; otherwise -1 and set 
 * error code to nioerrno.
 */
int far *nio_set_async_notify(int socket, int event, int (far *handler)(), u32 hint)
{
    x86regs_t r;
    r.ds = FP_SEG(handler);
    r.dx = FP_OFF(handler);
    r.es = (u16)(hint >> 16);
    r.di = (u16)(hint & 0xFFFFL);
    r.bx = socket;
    r.cx = event;
    r.ax = SET_ASYNC_NOTIFICATION;
    return (-1 == call_sock_dos_api(&r)) ?
                ((int far *)MK_FP(-1, -1)) : ((int far *)MK_FP(r.ds, r.dx));
}

/*! 
 */
int far nio_async_notify_handler(void)
{
    static int socket, event;
    static u16 arglo, arghi, hintlo, hinthi;
    static u32 arg;
    static FH h; //
    static int save_ss, save_sp;
    static char stack[HANDLER_STACK_SIZE];
    static int stack_pointer = (int)stack + HANDLER_STACK_SIZE - 10;
    static int save_neterrno, save_netsuberrno;

    __asm {
        // It is safe to use local variables as async notifications don't nest
        // We can safely ignore BP as long as we do not use any auto variables
        // Save arguments in registers into local variables
        mov socket, bx;
        mov event, cx;
        mov arglo, dx;
        mov arghi, si;
        mov hinthi, es;
        mov hintlo, di;
        // Save stack registers
        mov save_ss, ss;
        mov save_sp, sp;
        // Switch stack to local stack
        mov ax, ds;
        mov ss, ax;
        mov sp, stack_pointer;
    }

    // Save the error variables just incase we interrupted while they have
    // been set, but not used yet and our user calls api.
    save_neterrno = nioerrno;
    save_netsuberrno = niosuberrno;

    ((u16 *)&h)[0] = hintlo;
    ((u16 *)&h)[1] = hinthi;
    ((u16 *)&arg)[0] = arglo;
    ((u16 *)&arg)[1] = arghi;

    (*h)(socket, event, arg);

    __asm {
        // Restore stack registers
        mov ss, save_ss;
        mov sp, save_sp;
    }

    // resore error variables
    nioerrno = save_neterrno;
    niosuberrno = save_netsuberrno;
    return 0;
}

/*! 
 * Resolve IP-address from symbolic name.
 * \param zname Pointer to a string containing the symbolic name.
 * \param cname Pointer to the buffer receiving the canonical name.
 * \param cnamelen Buffer size is the canonical name.
 * \return On success IP address; otherwise 0 and set 
 * error code to nioerrno.
 */
u32 nio_resolve_name(char *zname, char *cname, int cnamelen)
{
    x86regs_t r;
    // Pointers in this module may be far or not, depending on the memory model used by
    // the compiler. The following code will ensure that long pointer is passed
    // to the Sockets completely null if the offset pointer is zero.
    r.dx = FP_OFF(zname);
    r.ds = (zname) ? (FP_SEG(zname)) : (0);
    r.di = FP_OFF(cname);
    r.es = (cname) ? (FP_SEG(cname)) : (0);
    r.cx = cnamelen;
    r.ax = RESOLVE_NAME;
    return (-1 == call_sock_dos_api(&r)) ? (0L) : (((u32)r.dx << 16L) + r.ax);
}

/*!
 * Gets the IP-address from decimal address.
 * \param zname Pointer to a string containing a decimal point with the address.
 * \return On success IP address; otherwise 0 and set 
 * error code to nioerrno.
 */
u32 nio_parse_address(char *zname)
{
    x86regs_t r;
    r.ds = FP_SEG(zname);
    r.dx = FP_OFF(zname);
    r.ax = PARSE_ADDRESS;
    return (-1 == call_sock_dos_api(&r)) ? (0L) : (((u32) r.dx << 16L) + r.ax);
}

/*! 
 * Sets the a \a socket options.
 * \param socket Socket descriptor.
 * \param level The level of properties. This value is ignored.
 * \param opt Settable property (NET_OPT_NON_BLOCKING, NET_OPT_TIMEOUT, NET_OPT_WAIT_FLUSH).
 * \param optval Value of the property.
 * \param len The size of the property value. In all cases, equal to 4.
 * \return On success global socket descriptor; otherwise -1 and set 
 * error code to nioerrno.
 */
int nio_set_opt(int socket, int level, int opt, u32 optval, int len)
{
    x86regs_t r;
    r.ds = (u16)(optval >> 16);
    r.dx = (u16)(optval & 0xFFFFL);
    r.bx = socket;
    r.cx = len;
    r.si = level;
    r.di = opt;
    r.ax = SET_OPTION;
    return call_sock_dos_api(&r);
}

/*!
 * Includes a socket to a multicast group.
 * \param groupaddr Group address for which is receiving multicast IP datagrams.
 * \param ifaceaddr IP-address to use interface. The first interface to be specified
 * in SOCKET.CFG is the default interface in the case when ifaceaddr == 0.
 * \return On success 0; otherwise any value of the integer containing the error code.
 */
int nio_join_group(u32 groupaddr, u32 ifaceaddr)
{
    x86regs_t r;
    group_addr_t gaddr;
    group_addr_t *pgaddr = &gaddr;
    gaddr.groupaddr = groupaddr;
    gaddr.ifaceaddr = ifaceaddr;
    r.ds = FP_SEG(pgaddr);
    r.si = FP_OFF(pgaddr);
    r.ax = JOIN_GROUP;
    return call_sock_dos_api(&r);
}

/*!
 * Removes the socket from the multicast group.
 * \param Groupaddr group address for which is receiving multicast IP datagrams.
 * \param ifaceaddr IP-address to use interface. The first interface to be specified
 * in SOCKET.CFG is the default interface in the case when ifaceaddr == 0.
 * \return On success 0; otherwise any value of the integer containing the error code.
 */
int nio_leave_group(u32 groupaddr, u32 ifaceaddr)
{
    x86regs_t r;
    group_addr_t gaddr;
    group_addr_t *pgaddr = &gaddr;
    gaddr.groupaddr = groupaddr;
    gaddr.ifaceaddr = ifaceaddr;
    r.ds = FP_SEG(pgaddr);
    r.si = FP_OFF(pgaddr);
    r.ax = LEAVE_GROUP;
    return call_sock_dos_api(&r);
}

/*! 
 * The function of control of asynchronous interfaces.
 * The function will return the combination IOCTL_GETSTATUS bit:
 * ST_DTR, ST_RTS, ST_CTS, ST_DSR, ST_RI, ST_DCD, ST_CONNECTED, ST_MODEMSTATE, 
 * STM_NONE, STM_IDLE, STM_INITIALIZING, STM_DIALING, STM_CONNECTING, STM_ANSWERING.
 * \param zname Pointer to the interface name.
 * \param fun The function to perform ioctl_functions_t.
 * \return On success >= 0; otherwise -1.
 */
int nio_ioctl(char *zname, ioctl_functions_t fun)
{
    x86regs_t r;
    r.ds = FP_SEG(zname);
    r.si = FP_OFF(zname);
    r.ax = IFACE_IOCTL + fun;
    return call_sock_dos_api(&r);
}

