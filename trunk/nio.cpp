/*********************************************************************************************
Project :
Version :
Date    : 30.03.2011
Author  : ������� �.�.
Company :
Comments: ���������� ��� ������ � ����� TCP/IP � ������������ ADAM 5510/TCP.
**********************************************************************************************/

/*! \file nio.cpp

  ������������ ������ (�����) "nio" - Network Input Output.

  � ���� ������ ���������� ��������� ��� ������� � ������ � ����� TCP/IP ��� ����� ADAM 5510/TCP.
*/

#include <dos.h>
#include "nio.h"


//--------------------------------------------------------------------------------------------------------//
/*** ��������� ���������� ***/

/*!
*/
enum {
    HANDLER_STACK_SIZE    = 500, /*!< ���� ������ ����� ��� ����������� �������� �������. */
    SOCKETS_API_INTERRUPT = 0x61 /*!< ��������� ���������� ������������, ����� �������� CallSocketsDosApi()
                                       �������, ��� ���������� Sockets ���� �����������.
                                       �� ��������� ��� 0x61, ���� ������������� � ��������� ������
                                       �������� SOCKETM ��� SOCKETSP. */
};

/*! \struct x86regs_t
    ��������� �������� Sockets ������������� ����� ����������� ���������� �
    ������� ��������������� ��������� ������������ �������,
    ��������� ���������, ���������� ��������� ��� �������� ������.
*/
typedef struct X86REGS {
    u16 ax; u16 bx; u16 cx; u16 dx;
    u16 si; u16 di; u16 ds; u16 es;
} x86regs_t;

//--------------------------------------------------------------------------------------------------------//
/*** ��������� ������� ***/

/*! \fn int CallSocketsDosApi(x86regs_t *x86r)
    �������� ������ ���� � ������� ������������ ���������� SOCKETS_API_INTERRUPT
    � ���������� ������������� � x86regs_t ���������.
    \param x86r ��������� �� x86regs_t ��������� ������� ����� ����
    ��������� ��� ������ Sockets ����������.
    ��� ��������� ��������� ����� �������� �������� ����� ���������� Sockets ����������.
    \return
    - ��� ������: � AX ���������� �������� Sockets ������
    - ��� ������: -1, ���� Sockets API �� ������
*/
static int CallSocketsDosApi(x86regs_t *x86r)
{
    union REGS r;
    struct SREGS s;
    nioerrno = niosuberrno = NO_ERR;
    /* Make certain the sockets interrupt is set */
    //if (!SOCKETS_API_INTERRUPT) {
    //    nioerrno = ERR_API_NOT_LOADED;
    //    return -1;
    //}
    // ����� ���������� � �������� ���������������� ������������� ��������� ��� Sockets API.
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
    /* ����� ������ ���������� � ������� ������ � ������� ������ � ������� 8-�� ����� �
    ����� ���-���� ������ � ������� 8-�� �����. */
    if (0x01 & r.x.cflag) {
        nioerrno = x86r->ax & 0x00FF;
        niosuberrno = x86r->ax >> 8;
        return -1;
    }
    return x86r->ax;
}

// ������ ������� ����������� ������� �������� �� ������.
typedef int (far *FH)(int, int, u32);

//--------------------------------------------------------------------------------------------------------//
/*** ���������� ���������� ***/

/*!
    �������� ��������� ���������� � ���������� ������,
    ������� ��������� ���������� ����������� ������� �� ����� ��������� � ����� Sockets.
    ��� ��������������� ����� �� ����� ������ � CAPI � ������������� ������ ���� ���������� ����� ������� ��� ��������.
*/
int nioerrno = NO_ERR;
int niosuberrno = NO_ERR;

//--------------------------------------------------------------------------------------------------------//
/*** ���������� ������� ***/

/*! \fn int nio_sockets_version(void)
    ���������� ����� ������ Sockets API.
    \return
    - ��� ������: ����� ������
    - ��� ������: -1, ���� ����� API �� ������
*/
int nio_sockets_version(void)
{
    x86regs_t r;
    r.ax = GET_SOCKETS_VERSION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_disable_async_notify(void)
    ��������� ����������� ����������� (�������� ������).
    \return
    - ��� ������: 0 - ���������, 1 - ��������
    - ��� ������: -1, ��� ��������� ���� ������ � ��� ���������� nioerrno � niosuberrno
*/
int nio_disable_async_notify(void)
{
    x86regs_t r;
    r.ax = DISABLE_ASYNC_NOTIFICATION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_enable_async_notify(void)
    �������� ����������� ����������� (�������� ������).
    \return
    - ��� ������: ���������� ��������� �����������
    - ��� ������: -1, ��� ��������� ���� ������ � ��� ���������� nioerrno � niosuberrno
*/
int nio_enable_async_notify(void)
{
    x86regs_t r;
    r.ax = ENABLE_ASYNC_NOTIFICATION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_is_socket(int socket)
    ��������� DOS-������������� ������ \a socket.
    \param socket ���������� ������.
    \return
    - ��� ������: 0
    - ��� ������: -1, ��� ��������� ���� ������ � ��� ���������� nioerrno � niosuberrno
*/
int nio_is_socket(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = IS_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_dcsocket(void)
    �������� DOS-����������� ���������� ������.
    ��� ������� �������� DOS, ����� ������� ���������� DOS �����.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � nioerrno.
*/
int nio_dcsocket(void)
{
    x86regs_t r;
    r.ax = GET_DC_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_socket(void)
    �������� ���������� ������.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_socket(void)
{
    x86regs_t r;
    r.ax = GET_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_version(void)
    ���������� ����� ������ ����������� API.
    \return
    - ��� ������: 0x0214
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_version(void)
{
    x86regs_t r;
    r.ax = GET_NET_VERSION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_convert_dcsocket(int socket)
    ����������� DOS ����������� ���������� ������ \a socket � ���������� ���������� ������.
    ��� ������� �������� DOS, ����� ������� ���������� DOS �����.
    \param socket DOS ���������� ������.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_convert_dcsocket(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = CONVERT_DC_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_eof(int socket)
    ��������� ����� (TCP) ���������� (�������� FIN).
    ����� ������ nio_eof(), ������ �������� nio_send().
    �����, ������, �������� �������� ��� ������ ���� ��� �� ������� ����������.
    \param socket ���������� ������.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_eof(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = EOF_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_flush(int socket)
    ������� (������������� ����������) ��� �������� ������ ����������� � ������� TCP ����������.
    \param socket ���������� ������.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_flush(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = FLUSH_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_release(int socket)
    ��������� ���������� � ����������� ��� �������.
    �� ����� ������ ������� (TCP) ����� ��� ������� ������ ���� ������� ������ ���� ��� ����� ���������� ����
    ������� � ����� ������, ����� (�������� ��������) ����� �������� � ������.
    \param socket ���������� ������.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_release(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = RELEASE_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_dcrelease(void)
    ��������� ��� ���������� � ����������� ��� �������, ��������������� � DOS-������������ ��������.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_dcrelease(void)
{
    x86regs_t r;
    r.ax = RELEASE_DC_SOCKETS;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_abort(int socket)
    ��������� ����������� � ���� � ����������� ��� �������.
    ��� ������� �������� ��������������� ������� ����� �� ��������� ����������.
    \param socket ���������� ������.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_abort(int socket)
{
    x86regs_t r;
    r.ax = ABORT_SOCKET;
    r.bx = socket;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_dcabort(void)
    ��������� ��� DOS-����������� �����������.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_dcabort(void)
{
    x86regs_t r;
    r.ax = ABORT_DC_SOCKETS;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_shutdown(void)
    ��������� ������ ���� � ��������� Sockets TCP/IP ����.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_shutdown(void)
{
    x86regs_t r;
    r.ax = SHUT_DOWN_NET;
    return CallSocketsDosApi(&r);
}

/*! \fn u32 nio_address(int socket)
    ���������� ��������� ����� IP-����������.
    � ������ ������ ����-���������� - ��� IP-����� �����.
    � ������ ����� ������ ���������� IP-����� ���������� ������������ ��� ������������� ������� ��� ������������ �����������.
    \param socket ���������� ������.
    \return
    - ��� ������: IP �����
    - ��� ������: 0, � ���������� ���� ������ � nioerrno.
*/
u32 nio_address(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = GET_ADDRESS;
    return (-1 == CallSocketsDosApi(&r)) ? (0L) : (((u32)r.dx << 16L) + r.ax);
}

/*! \fn int nio_peer_address(int socket, net_addr_t *na)
    ���������� ���������� \a na �� ������ ���� ������������� ������ \a socket.
    \param socket ���������� ������.
    \param na ��������� ���������� ���� net_addr_t � ������� ������������ ����� ������������� ����.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � nioerrno.
*/
int nio_peer_address(int socket, net_addr_t *na)
{
    x86regs_t r;
    r.ds = FP_SEG(na);
    r.dx = FP_OFF(na);
    r.bx = socket;
    r.ax = GET_PEER_ADDRESS;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_kernel_info(int reserved, kernel_info_t code, u8 devid, void *data, u16 *size)
    ���������� ������������� ���������� �� ����.
    \param reserved ���������������� � �� ������������, ������ ���� ����������� � 0.
    \param code ��� ������������� ���������� � ���� ���� kernel_info_t.
    \param devid ???.
    \param data ��������� �� ������� ������ � ������� ������������ ���������� ����.
    \param size ��������� �� ������� ������ � ������� ������������ ������ ���������� ����.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_kernel_info(int reserved, kernel_info_t code, u8 devid, void *data, u16 *size)
{
    x86regs_t r;
    /*
    ��������� � ������ ����� ������ ����� ���� �������� ��� ��� � ����������� �� ������ ������ ������������ ������������.
    ��������� ��� ����� �������������, ��� ������� ��������� ���������� � Sockets ��������� �������,
    ���� �������� ��������� ����� ����.
    */
    r.si = FP_OFF(data);
    r.ds = (data) ? (FP_SEG(data)) : (0);
    r.di = FP_OFF(size);
    r.es = (size) ? (FP_SEG(size)) : (0);
    r.bx = reserved;
    r.dx = code + devid;
    r.ax = GET_KERNEL_INFO;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_icmp_ping(u32 host, int length)
    ���������� ���� ICMP (���-������) � ���� ���� �� ����� ������� �����
    ��� � ������� ����� ������, ���� ����� �� �������.
    ������� nio_icmp_ping() ������ �����������!
    \param host IP-����� ����� ��� �����.
    \param length ���������� ���� ������ � ������� �����.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_icmp_ping(u32 host, int length)
{
    x86regs_t r;
    r.bx = (u16)(host & 0xFFFFL);
    r.dx = (u16)(host >> 16);
    r.cx = length;
    r.ax = ICMP_PING;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_kernel_cfg(kernel_config_t *kc)
    ���������� � \a kc ������������ ����.
    \param kc ��������� �� ��������� ������� ���� ���� kernel_config_t.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_kernel_cfg(kernel_config_t *kc)
{
    x86regs_t r;
    r.ds = FP_SEG(kc);
    r.si = FP_OFF(kc);
    r.ax = GET_KERNEL_CONFIG;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_info(int socket, net_info_t *ni)
    ���������� � \a ni ���������� � ����.
    \param socket ���������� ������.
    \param ni ��������� �� ��������� ���� net_info_t.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
*/
int nio_info(int socket, net_info_t *ni)
{
    x86regs_t r;
    r.ds = FP_SEG(ni);
    r.si = FP_OFF(ni);
    r.bx = socket;
    r.ax = GET_NET_INFO;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_connect(int socket, connection_types_t ntype, net_addr_t *na)
    ������� ����������� � ����.
    ���� ����� \a socket ����������� ��� -1, DOS ����������� ������ �������������.
    � ���� ������ ������ DOS ��������� ���������� �����.
    ���� ����� \a socket ������ ��� ������������� ��� \a ntype ������ ��� DATAGRAM ��� ����� ��
    ���� ����� ������������ ����������.
    � ������ STREAM ���������� ����� ����� ���� ��� �� ����������� � nio_recv() ����� ���� �����������
    ��� �������� ������������ ����������.
    ���� nio_recv() ���������� ��� ERR_NOT_ESTAB ���������� �� �����������,
    ������� ������� ��� ������ ERR_WOULD_BLOCK ��� �������� ��������� �� �� ��� ���������� �����������.
    ����� ������� ����� ���������� SetAsyncNotify() � NET_AS_OPEN ��� �������� ��� ������������ ����������.
    NET_AS_ERROR ����� ������ ���� ����������� ��� ����������� � ��������� ������� ��������.
    \param socket ���������� ������.
    \param ntype ��� ����������� connection_types_t.
    \param na ��������� �� ��������� ���� net_addr_t.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_listen(int socket, connection_types_t ntype, net_addr_t *na)
    ������������ ������� �����������.
    ���� ����� ����������� ��� -1 �� ������������� DOS ����������� �����.
    � ���� ������ ������ DOS ��������� ���������� �����.
    ���� ����� \a socket ������ ��� ������������� ��� ��� ����� \a ntype ������ ��� DATAGRAM
    �� ���� ����� ������������ ����������.
    � ������ ���������� ���� STREAM ���������� ����� ���� ��� �� �����������.
    nio_recv() ����� ���� ����������� ��� �������� ������������ ����������.
    ���� nio_recv() ���������� ��� ERR_NOT_ESTAB, ���������� �� �����������.
    �������� ����������� ��� ������ ERR_WOULD_BLOCK ��������� �� �� ��� ���������� �����������.
    ����� ������� ����� ���������� SetAsyncNotify() � NET_AS_OPEN ��� �������� ��� ������������ ����������.
    NET_AS_ERROR ����� ������ ���� ����������� ��� ����������� � ��������� ������� ��������.
    \param socket ���������� ������.
    \param ntype ��� ����������� connection_types_t.
    \param na ��������� �� ��������� ���� net_addr_t.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_select(int maxid, long *iflags, long *oflags)
    �������� ���� DOS-����������� ������� ��� ����������� ������ ��� ������ � ������.
    32-��������� \a iflags ������������ ����� 32 DC ������ ����������� ��� ������� ������ � ������ ������,
    � ������ 32-��������� \a oflags ��� ������� DC ������� ��� ������.
    �������� �������� ��� ������������ ����� �� ��������� 0 � ����� ������� ��� ������������ ����� �� ��������� 31.
    ���� �������������� ���������������� �������, �������� ��� ���������.
    �������� ���������� \a iflags � \a oflags ����������� �������� ���������� �������� �������.
    \param maxid ���������� ����������� �������.
    \param iflags ��������� �� ������� �����, ������������ ���������� ��� ������ ������.
    \param oflags ��������� �� �������� �����, ������������ ���������� ��� �������� ������.
    \return
    - ��� ������: 0
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_recv(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
    ������ �� ���� � ������� �������.
    ������������ ��� ������ ����� ��������� ����� ������ �������� ���������� �� ������������ ���������.
    ���� �������� �� �������� ����������� (����� ��������������� nio_set_opt() � NET_OPT_NON_BLOCKING ������
    ��� ������� ����� � NET_FLG_NON_BLOCKING), nio_recv() ���������� ���������� ���-�� ��������� ������ ��� ������ ERR_WOULD_BLOCK.
    � ������ STREAM (TCP) ������, ������ ������ �� ����������, � ����� ���������� ������ ����� ���� ��������� � ����� �����,
    ���������� �� ���� �� �������� �� ��� ��������� �� ����.
    ��� ������ ��������� ��� ��������, ���� ���� ������ ������, ��� ������ ������ ��������.
    ��, ��� �� �������� �� ���� �����, ������������ �� ����������� ������.
    ���� NULL ��������� �����, ���������� ���� �� ������� �������� ������������.
    � ������ DATAGRAM (UDP) ������, ��� ����������� ������������ � ���� �����, ���� ����� ������� ���,
    � � ���� ������ ������ ����� �������, ��� ����� �������� ������ ������.
    ��������� ������ ����� ��������.
    ���� ������ �������� � ��� NET_FLG_PEEK � NET_FLG_NON_BLOCKING ����� �� �������,
    ����� ���������� �� ������� �������� ������������.
    ���� ������ �������� � NET_FLG_PEEK ����������� ??� NULL ��������� �����,
    ���������� ���� � ��������� ���������� ������������.
    \param socket ���������� ������.
    \param buf ��������� �� ����� � ������� ����� ��������� ������.
    \param len ������ ������, �.�. ������������ ���-�� ���� ��� ������.
    \param na ��������� �� ��������� net_addr_t ���������� �������� ���������� � ��������� � ��������� ������ � ��������� IP-�������.
    \param flags �����, ������������ ���������������� (����� ���������� �� NET_FLG_PEEK � NET_FLG_NON_BLOCKING).
    \return
    - ��� ������: ���������� ����������� ����
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo, ��� �������� 0 ��������, ��� ��� ��������� ����������.
*/
int nio_recv(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    /*
    ��������� � ������ ����� ������ ����� ���� �������� ��� ��� � ����������� �� ������ ������ ������������ ������������.
    ��������� ��� ����� �������������, ��� ������� ��������� ���������� � Sockets ��������� �������,
    ���� �������� ��������� ����� ����.
    */
    r.di = FP_OFF(na);
    r.es = (na) ? (FP_SEG(na)) : (0);
    r.si = FP_OFF(buf);
    r.ds = (buf) ? (FP_SEG(buf)) : (0);
    r.dx = flags;
    r.cx = len;
    r.bx = socket;
    r.ax = READ_SOCKET;
    return (-1 == CallSocketsDosApi(&r)) ? (-1) : (r.cx);
}

/*! \fn int nio_recv_from(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
    ������ �� ���� � ������� ������� � ������������ ������ ��� ������������� ��� DATAGRAM �������.
    ��� ��������� � IP-������� � ������ ���������� ��������������� �������� � ��������� net_addr_t � �� ����� ��� ������ �������������.
    ������� �������� ��� remote_host ������������ � �������� �������, ����� �������� � ������ ����� �
    ������� �������� ��� remote_port ������������ � �������� �������, ����� �������� �� ������ �����.
    ��������� ����, local_port, �� ����� ���� ������� ��� �������.
    � ������ ���������� nio_recv_from() ����� ���� ��� ��, ��� nio_recv().
    \param socket ���������� ������.
    \param buf ��������� �� ����� � ������� ����� ��������� ������.
    \param len ������ ������, �.�. ������������ ���-�� ���� ��� ������.
    \param na ��������� �� ��������� net_addr_t ���������� �������� ���������� � ��������� � ��������� ������ � ��������� IP-�������.
    \param flags �����, ������������ ���������������� (����� ���������� �� NET_FLG_PEEK � NET_FLG_NON_BLOCKING).
    \return
    - ��� ������: ���������� ����������� ����
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo, ��� �������� 0 ��������, ��� ���������� ��������� ����������.
    .
    \note �������� �������� �� ��������� ��������: ���� ���������� ���������,
    ������� � ����� ������ ErrWouldBlock ���������� ��������� � ������ ��������, ��� ������ �� � ��������� �����.
*/
int nio_recv_from(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    /*
    ��������� � ������ ����� ������ ����� ���� �������� ��� ��� � ����������� �� ������ ������ ������������ ������������.
    ��������� ��� ����� �������������, ��� ������� ��������� ���������� � Sockets ��������� �������,
    ���� �������� ��������� ����� ����.
    */
    r.di = FP_OFF(na);
    r.es = (na) ? (FP_SEG(na)) : (0);
    r.si = FP_OFF(buf);
    r.ds = (buf) ? (FP_SEG(buf)) : (0);
    r.dx = flags;
    r.cx = len;
    r.bx = socket;
    r.ax = READ_FROM_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_send(int socket, char *buf, u16 len, u16 flags)
    ������ � ���� � ������� �������.
    \param socket ���������� ������.
    \param buf ��������� �� ����� ���������� ������ ��� ��������.
    \param len ������ ������, �.�. ���-�� ���� ��� ��������.
    \param flags �����, ������������ ����������������
            (����� ���������� �� NET_FLG_OOB, NET_FLG_PUSH, NET_FLG_NON_BLOCKING, NET_FLG_BROADCAST, NET_FLG_MC_NOECHO).
    \return
    - ��� ������: ���������� ���������� ����
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
    .
    \note ���-�� ���� ���������� ���������� ��� ������������� ������, ����� ���� ������, ��� ����� \a len.
    � ����� ������, �������� �������������� ������ ����� ���������, ��������������� � ��������� ���������.
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_send_to(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
    ������ � ���� � ������� ������� ������� (UDP ������).
    \param socket ���������� ������.
    \param buf ��������� �� ����� ���������� ������ ��� ��������.
    \param len ������ ������, �.�. ���-�� ���� ��� ��������.
    \param na ��������� �� ��������� net_addr_t, ���������� ��������� ����, ����� �������� � � ���������� ����� � IP-����� ��� ������.
    \param flags �����, ������������ ����������������
            (����� ���������� �� NET_FLG_NON_BLOCKING, NET_FLG_BROADCAST).
    \return
    - ��� ������: ���������� ���������� ����
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_set_alarm(int socket, u32 time, int (far *handler)(), u32 hint)
    ������������� ��������� ������???
    \param socket ���������� ������.
    \param time �������� ������� � �������������.
    \param handler ������� ����� ������������ ��������� ������.
    �������� �������� nio_set_async_notify() ��� ������� ������� ��������� ������.
    \param hint ��������, ������������ ������� ��������� ������.
    \return
    - ��� ������: ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return CallSocketsDosApi(&r);
}

/*! \fn int far *nio_set_async_notify(int socket, int event, int (far *handler)(), u32 hint)
    ������������� ����������� ����������� (�������� �����) ��� ����������� �������.
    ������ ������� CAPI ����� ���� ������� � ��������� ������, �� ����������� nio_resolve_name ),
    ������� ����� ������� DOS.
    ��������� ������ �� ��������� � C �������� � ����� ��������� � ��������� ������������� ������ ���� �������.
    ��������� ����������� ��������� ���������� �� ���������.
    ��� ����� ���� ������� ����� ������ �� �������� ����������, �����, ��� _BX, ��� � ������� ���������� ����������.
    � ��������� ������, ���� ������������ ������ � ����� ���� ������ ��� � ����������� ��
    / � = �������� ��������� ������ ��� �������� Sockets.
    ������� �����, ��������, �� ����� �������� ������, ������� ����� ������� �������� ��� ����� ������,
    ������ ��� ������� ������ ������ ������������.
    ����� ������� ������ ������� ���� �������� ����������� � ������������� ����������, ������� ��� �������� ������ ������.
    ������ �������� - ������������� DS = SS �������� ����������� ��� ������� ���� ������������� �� ������� ������ �����!.
    ���� ��������� ������ ������� �� � ��� C + +, _loadds ����������� ����� ���� ����������� ��� ��������� �������� ������,
    ��� � ������, ������� ��������� DS ������������ ��� ���������� ���������.
    (��� ������ DS == SI �� ����� ��� Sockets ������ 1.04 � ����.)
    �������������� ����� ����������� � ������������� ��������� ���������� nio_set_async_notify() � ES: DI,
    ��� ��������� �� ���������, ������� �������� ��� � �������� ��� � ��������� ������.
    ���� DS �� ���������� �� ������� ������ ������, �� ������� � CAPI.C �� ��������: �� ������������ �� � ��������.
    ��������� ������, ��������, ����� ����������� � ������ ���������� ��� �������� ����������� ��� DOS.
    �� ������������ ����� �������, ����� ��� ������� () ��� � (), � ������� ��������� ������, ������� ����� ������� DOS ��� ������.
    ��� ������� �������� ���������������� ������ ��� ����� ������ � �������� �����.
    ��������� ������ �������, ������� �������� �������� � ����� ���������� ������� �������������.
    ������� ��������� ������ �� ���������.
    ������� ��������� ������ �� ���������� � �� ����� ��� �������� ����� ������������, ���� ���� ������ ������� CAPI ����������.
    ��� ��������� �������� � ������� 2, 3 � 4 ����, ���������� ���������� � CAPI.C �������
    ���������� ��������� �������� ��� �������� ������ C-����������� ����������,
    � ������� ������, ������� �������� ����� C-�����������.
    ���� ���������� ������ nio_async_notify_handler.
    ������������ ���������� � ������ MyHandler ����, ���������� � ������� ������� �� ������ � 500 ����.
    ������ ����� �������� ����� ���� ����������� ����� ��������� HANDLER_STACK_SIZE ���������� � CAPI.C.
    \param socket ���������� ������.
    \param event ������� ������� ����� ���� ����������� �� net_async_notify_route_t.
    \param handler ������� ����� ������� ��������� ������.
    \param hint ��������, ������������ ������� ��������� ������.
    \return
    - ��� ������: ��������� �� ���������� ����������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return (-1 == CallSocketsDosApi(&r)) ?
                ((int far *)MK_FP(-1, -1)) : ((int far *)MK_FP(r.ds, r.dx));
}

/*! \fn int far nio_async_notify_handler(void)
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
    // been set, but not used yet and our user calls CAPI.
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

/*! \fn u32 nio_resolve_name(char *zname, char *cname, int cnamelen)
    ��������� IP-����� �� �������������� �����.
    \param zname ��������� �� ������ ���������� ������������� ���.
    \param cname ��������� �� ����� ����������� ������������ ���.
    \param cnamelen ������ ������ ������������ �� ������������ ���.
    \return
    - ��� ������: IP �����
    - ��� ������: 0, � ���������� ���� ������ � SocketsErrNo
*/
u32 nio_resolve_name(char *zname, char *cname, int cnamelen)
{
    x86regs_t r;
    /*
    ��������� � ������ ����� ������ ����� ���� �������� ��� ��� � ����������� �� ������ ������ ������������ ������������.
    ��������� ��� ����� �������������, ��� ������� ��������� ���������� � Sockets ��������� �������,
    ���� �������� ��������� ����� ����.
    */
    r.dx = FP_OFF(zname);
    r.ds = (zname) ? (FP_SEG(zname)) : (0);
    r.di = FP_OFF(cname);
    r.es = (cname) ? (FP_SEG(cname)) : (0);
    r.cx = cnamelen;
    r.ax = RESOLVE_NAME;
    return (-1 == CallSocketsDosApi(&r)) ? (0L) : (((u32)r.dx << 16L) + r.ax);
}

/*! \fn u32 nio_parse_address(char *zname)
    �������� IP-����� �� ����������� ������.
    \param zname ��������� �� ������ ���������� ���������� ����� � ������.
    \return
    - ��� ������: IP �����
    - ��� ������: 0, � ���������� ���� ������ � SocketsErrNo
*/
u32 nio_parse_address(char *zname)
{
    x86regs_t r;
    r.ds = FP_SEG(zname);
    r.dx = FP_OFF(zname);
    r.ax = PARSE_ADDRESS;
    return (-1 == CallSocketsDosApi(&r)) ? (0L) : (((u32) r.dx << 16L) + r.ax);
}

/*! \fn int nio_set_opt(int socket, int level, int opt, u32 optval, int len)
    ������������� ��������� ������.
    \param socket ���������� ������.
    \param level ������� ��������. ��� �������� ������������.
    \param opt ��������������� �������� (NET_OPT_NON_BLOCKING, NET_OPT_TIMEOUT, NET_OPT_WAIT_FLUSH).
    \param optval �������� ��������.
    \param len ������ �������� ��������. �� ���� ������� ����� 4-�.
    \return
    - ��� ������: ���������� ���������� ������
    - ��� ������: -1, � ���������� ���� ������ � SocketsErrNo
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_join_group(u32 groupaddr, u32 ifaceaddr)
    �������� ����� � ������ ������������� ��������.
    \param groupaddr ��������� ����� �� �������� ���� ��������� ��������� ����������.
    \param ifaceaddr IP-����� ��� ������������� ����������.
    ������ ��������� ������� ����� ������ � SOCKET.CFG �������� ����������� �� ��������� � ������ ����� ifaceaddr == 0.
    \return
    - ��� ������: 0
    - ��� ������: ����� ������ �������� ������ ���������� ��� ������.
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_leave_group(u32 groupaddr, u32 ifaceaddr)
    ��������� ����� �� ������ ������������� ��������.
    \param groupaddr ��������� ����� �� �������� ���� ��������� ��������� ����������.
    \param ifaceaddr IP-����� ��� ������������� ����������.
    ������ ��������� ������� ����� ������ � SOCKET.CFG �������� ����������� �� ��������� � ������ ����� ifaceaddr == 0.
    \return
    - ��� ������: 0
    - ��� ������: ����� ������ �������� ������ ���������� ��� ������.
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
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_ioctl(char *zname, ioctl_functions_t fun)
    ������� ���������� ������������ ������������.
    \param zname ��������� �� ��� ����������.
    \param fun ������� ��� ���������� ioctl_functions_t.
    \return
    - ��� ������: >= 0
    - ��� ������: -1.
    .
    ������� IOCTL_GETSTATUS ��������� ��������� ���:
    ST_DTR, ST_RTS, ST_CTS, ST_DSR, ST_RI, ST_DCD, ST_CONNECTED,
    ST_MODEMSTATE, STM_NONE, STM_IDLE, STM_INITIALIZING, STM_DIALING, STM_CONNECTING, STM_ANSWERING.
*/
int nio_ioctl(char *zname, ioctl_functions_t fun)
{
    x86regs_t r;
    r.ds = FP_SEG(zname);
    r.si = FP_OFF(zname);
    r.ax = IFACE_IOCTL + fun;
    return CallSocketsDosApi(&r);
}
