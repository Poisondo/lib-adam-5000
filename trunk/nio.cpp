/*********************************************************************************************
Project :
Version :
Date    : 30.03.2011
Author  : Шиенков Д.И.
Company :
Comments: Библиотека для работы с сетью TCP/IP в контроллерах ADAM 5510/TCP.
**********************************************************************************************/

/*! \file nio.cpp

  Аббревиатура модуля (файла) "nio" - Network Input Output.

  В этом модуле реализован интерфейс для доступа и работы с сетью TCP/IP ПЛК серии ADAM 5510/TCP.
*/

#include <dos.h>
#include "nio.h"


//--------------------------------------------------------------------------------------------------------//
/*** ЛОКАЛЬНЫЕ ПЕРЕМЕННЫЕ ***/

/*!
*/
enum {
    HANDLER_STACK_SIZE    = 500, /*!< Стек размер стека для асинхронных обратных вызовов. */
    SOCKETS_API_INTERRUPT = 0x61 /*!< Следующие глобальные используется, чтобы сообщить CallSocketsDosApi()
                                       функции, что прерывание Sockets стек подключения.
                                       По умолчанию это 0x61, если переопределен в командной строке
                                       нагрузка SOCKETM или SOCKETSP. */
};

/*! \struct x86regs_t
    Интерфейс драйвера Sockets осуществлямый через программное прерывание с
    набором соответствующих регистров обозначающих функции,
    различные параметры, передающих указатели для передачи данных.
*/
typedef struct X86REGS {
    u16 ax; u16 bx; u16 cx; u16 dx;
    u16 si; u16 di; u16 ds; u16 es;
} x86regs_t;

//--------------------------------------------------------------------------------------------------------//
/*** ЛОКАЛЬНЫЕ ФУНКЦИИ ***/

/*! \fn int CallSocketsDosApi(x86regs_t *x86r)
    Вызывает сокеты ядра с помощью программного прерывания SOCKETS_API_INTERRUPT
    с регистрами определенными в x86regs_t структуре.
    \param x86r Указатель на x86regs_t структуру которая имеет поля
    регистров для вызова Sockets прерываний.
    Эта структура обновляет новые значения регистра после выполнения Sockets прерывания.
    \return
    - при успехе: в AX возвращает значение Sockets вызова
    - при ошибке: -1, если Sockets API не найден
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
    // Вызов прерывания с сокетами соответствующими установленным регистрам для Sockets API.
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
    /* Любые ошибки сообщаемые с набором флагов и сетевые ошибки в младних 8-ми битах и
    любые под-коды ошибок в старших 8-ми битах. */
    if (0x01 & r.x.cflag) {
        nioerrno = x86r->ax & 0x00FF;
        niosuberrno = x86r->ax >> 8;
        return -1;
    }
    return x86r->ax;
}

// Шаблон функции обработчика который вызываем по адресу.
typedef int (far *FH)(int, int, u32);

//--------------------------------------------------------------------------------------------------------//
/*** ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ***/

/*!
    Публично доступные переменные с значениями ошибок,
    которые описывают дальнейшие подробности неудачи во время разговора с ядром Sockets.
    Они устанавливаются сразу же после вызова в CAPI и действительны только если предыдущий вызов функции был неудачен.
*/
int nioerrno = NO_ERR;
int niosuberrno = NO_ERR;

//--------------------------------------------------------------------------------------------------------//
/*** ГЛОБАЛЬНЫЕ ФУНКЦИИ ***/

/*! \fn int nio_sockets_version(void)
    Возвращает номер версии Sockets API.
    \return
    - при успехе: номер версии
    - при ошибке: -1, если номер API не найден
*/
int nio_sockets_version(void)
{
    x86regs_t r;
    r.ax = GET_SOCKETS_VERSION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_disable_async_notify(void)
    Отключает асинхронные уведомления (обратные вызовы).
    \return
    - при успехе: 0 - отключено, 1 - включено
    - при ошибке: -1, при установке кода ошибки в обе переменные nioerrno и niosuberrno
*/
int nio_disable_async_notify(void)
{
    x86regs_t r;
    r.ax = DISABLE_ASYNC_NOTIFICATION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_enable_async_notify(void)
    Включает асинхронные уведомления (обратные вызовы).
    \return
    - при успехе: предыдущее состояние уведомления
    - при ошибке: -1, при установке кода ошибки в обе переменные nioerrno и niosuberrno
*/
int nio_enable_async_notify(void)
{
    x86regs_t r;
    r.ax = ENABLE_ASYNC_NOTIFICATION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_is_socket(int socket)
    Проверяет DOS-совместимость сокета \a socket.
    \param socket Дескриптор сокета.
    \return
    - при успехе: 0
    - при ошибке: -1, при установке кода ошибки в обе переменные nioerrno и niosuberrno
*/
int nio_is_socket(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = IS_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_dcsocket(void)
    Получает DOS-совместимый дескриптор сокета.
    Эта функция вызывает DOS, чтобы открыть дескриптор DOS файла.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в nioerrno.
*/
int nio_dcsocket(void)
{
    x86regs_t r;
    r.ax = GET_DC_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_socket(void)
    Получает дескриптор сокета.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_socket(void)
{
    x86regs_t r;
    r.ax = GET_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_version(void)
    Возвращает номер версии совместимых API.
    \return
    - при успехе: 0x0214
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_version(void)
{
    x86regs_t r;
    r.ax = GET_NET_VERSION;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_convert_dcsocket(int socket)
    Преобразует DOS совместимый дескриптор сокета \a socket в нормальный дескриптор сокета.
    Эта функция вызывает DOS, чтобы закрыть дескриптор DOS файла.
    \param socket DOS дескриптор сокета.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_convert_dcsocket(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = CONVERT_DC_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_eof(int socket)
    Закрывает поток (TCP) соединения (посылает FIN).
    После вызова nio_eof(), нельзя вызывать nio_send().
    Сокет, однако, остается открытым для чтения пока пир не закроет соединение.
    \param socket Дескриптор сокета.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_eof(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = EOF_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_flush(int socket)
    Очищает (принудительно отправляет) все выходные данные находящиеся в очереди TCP соединения.
    \param socket Дескриптор сокета.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_flush(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = FLUSH_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_release(int socket)
    Закрывает соединение и освобождает все ресурсы.
    Во время обмена данными (TCP) связи эта функция должна быть вызвана только один раз чтобы соединение было
    закрыто с обеих сторон, иначе (неловкое закрытие) может привести к сбросу.
    \param socket Дескриптор сокета.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_release(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = RELEASE_SOCKET;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_dcrelease(void)
    Закрывает все соединения и освобождает все ресурсы, ассоциированные с DOS-совместимыми сокетами.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_dcrelease(void)
{
    x86regs_t r;
    r.ax = RELEASE_DC_SOCKETS;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_abort(int socket)
    Прерывает подключение к сети и освобождает все ресурсы.
    Эта функция вызывает непредсказуемый близкий сброс на потоковом соединении.
    \param socket Дескриптор сокета.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_abort(int socket)
{
    x86regs_t r;
    r.ax = ABORT_SOCKET;
    r.bx = socket;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_dcabort(void)
    Прерывает все DOS-совместимые подключения.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_dcabort(void)
{
    x86regs_t r;
    r.ax = ABORT_DC_SOCKETS;
    return CallSocketsDosApi(&r);
}

/*! \fn int nio_shutdown(void)
    Завершает работу сети и выгружает Sockets TCP/IP ядро.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_shutdown(void)
{
    x86regs_t r;
    r.ax = SHUT_DOWN_NET;
    return CallSocketsDosApi(&r);
}

/*! \fn u32 nio_address(int socket)
    Возвращает локальный адрес IP-соединения.
    В случае одного хост-интерфейса - это IP-адрес хоста.
    В случае более одного интерфейса IP-адрес интерфейса используется для маршрутизации трафика для определенных подключений.
    \param socket Дескриптор сокета.
    \return
    - при успехе: IP адрес
    - при ошибке: 0, с установкой кода ошибки в nioerrno.
*/
u32 nio_address(int socket)
{
    x86regs_t r;
    r.bx = socket;
    r.ax = GET_ADDRESS;
    return (-1 == CallSocketsDosApi(&r)) ? (0L) : (((u32)r.dx << 16L) + r.ax);
}

/*! \fn int nio_peer_address(int socket, net_addr_t *na)
    Возвращает информацию \a na об адресе пира подключенного сокета \a socket.
    \param socket Дескриптор сокета.
    \param na Структура информации типа net_addr_t в которую возвращается адрес подключенного пира.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в nioerrno.
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
    Возвращает специфицескую информацию из ядра.
    \param reserved Зарезервированно и не используется, должно быть установлено в 0.
    \param code Код запрашиваемой информации у ядра типа kernel_info_t.
    \param devid ???.
    \param data Указатель на область данных в которую возвращается информация ядра.
    \param size Указатель на область данных в которую возвращается размер информация ядра.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
*/
int nio_kernel_info(int reserved, kernel_info_t code, u8 devid, void *data, u16 *size)
{
    x86regs_t r;
    /*
    Указатели в рамках этого модуля могут быть дальними или нет в зависимости от модели памяти используемой компилятором.
    Следующий код будет гарантировать, что дальний указатель передается в Sockets полностью нулевым,
    если смещение указателя равно нулю.
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
    Отправляет пинг ICMP (эхо-запрос) и ждет пока не будет получен ответ
    или в течение шести секунд, если ответ не получен.
    Функция nio_icmp_ping() всегда блокирующая!
    \param host IP-адрес хоста для пинга.
    \param length Количество байт данных в запросе пинга.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Возвращает в \a kc конфигурацию ядра.
    \param kc Указатель на структуру конфига ядра типа kernel_config_t.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Возвращает в \a ni информацию о сети.
    \param socket Дескриптор сокета.
    \param ni Указатель на структуру типа net_info_t.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Создает подключение к сети.
    Если сокет \a socket указывается как -1, DOS совместимый разъем присваивается.
    В этом случае только DOS открывает дескриптор файла.
    Если сокет \a socket указан как неблокирующий или \a ntype указан как DATAGRAM тип связи то
    этот вызов возвращается немедленно.
    В случае STREAM соединения связь может быть еще не установлена и nio_recv() может быть использован
    для проверки установления соединения.
    Пока nio_recv() возвращает код ERR_NOT_ESTAB соединение не установлено,
    хороший возврат или ошибка ERR_WOULD_BLOCK при возврате указывает на то что соединение установлено.
    Более сложный метод использует SetAsyncNotify() с NET_AS_OPEN для проверки для установления соединения.
    NET_AS_ERROR также должны быть установлены для уведомления о неудачной попытке открытия.
    \param socket Дескриптор сокета.
    \param ntype Тип подключения connection_types_t.
    \param na Указатель на структуру типа net_addr_t.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Прослушивает сетевые подключения.
    Если сокет указывается как -1 то присваивается DOS совместимый сокет.
    В этом случае только DOS открывает дескриптор файла.
    Если сокет \a socket указан как неблокирующий или тип связи \a ntype указан как DATAGRAM
    то этот вызов возвращается немедленно.
    В случае соединения типа STREAM соединение может быть еще не установлено.
    nio_recv() может быть использован для проверки установления соединения.
    Пока nio_recv() возвращает код ERR_NOT_ESTAB, соединение не установлено.
    Успешное возвращение или ошибка ERR_WOULD_BLOCK указывает на то что соединение установлено.
    Более сложный метод использует SetAsyncNotify() с NET_AS_OPEN для проверки для установления соединения.
    NET_AS_ERROR также должны быть установлены для уведомления о неудачной попытке открытия.
    \param socket Дескриптор сокета.
    \param ntype Тип подключения connection_types_t.
    \param na Указатель на структуру типа net_addr_t.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Проверка всех DOS-совместимых сокетов для доступности данных для чтения и записи.
    32-разрядные \a iflags представляют собой 32 DC сокеты заполняется для каждого сокета с приема данных,
    а другой 32-разрядной \a oflags для сокетов DC готовых для записи.
    Наименее значащий бит представляет сокет со значением 0 и самый старший бит представляет сокет со значением 31.
    Биты представляющих неиспользованный розетки, остаются без изменений.
    Выходные переменные \a iflags и \a oflags заполняются текущими значениями статусов сокетов.
    \param maxid Количество проверяемых сокетов.
    \param iflags Указатель на входные флаги, индицирующие готовность для приема данных.
    \param oflags Указатель на выходные флаги, индицирующие готовность для передачи данных.
    \return
    - при успехе: 0
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Читает из сети с помощью сокетов.
    Возвращается как только любой ненулевой объем данных доступен независимо от блокирующего состояния.
    Если операция не является блокирующей (когда воспользовались nio_set_opt() с NET_OPT_NON_BLOCKING опцией
    или указали флаги с NET_FLG_NON_BLOCKING), nio_recv() возвращает немедленно кол-во имеющихся данных или ошибку ERR_WOULD_BLOCK.
    В случае STREAM (TCP) сокета, запись границ не существует, и любое количество данных может быть прочитана в любое время,
    независимо от пути по которому он был отправлен на пиры.
    Нет данных урезается или потеряны, даже если больше данных, чем размер буфера доступна.
    То, что не вернулся на один вызов, возвращается на последующие вызовы.
    Если NULL определен буфер, количество байт на очереди получают возвращается.
    В случае DATAGRAM (UDP) сокета, все дейтаграммы возвращается в один вызов, если буфер слишком мал,
    и в этом случае данные будут усечены, тем самым сохраняя записи границ.
    Усеченный данные будут потеряны.
    Если данные доступны и оба NET_FLG_PEEK и NET_FLG_NON_BLOCKING флаги не указаны,
    число дейтаграмм по очереди получают возвращается.
    Если данные доступны и NET_FLG_PEEK установлена ??и NULL определен буфер,
    количество байт в следующей датаграммы возвращается.
    \param socket Дескриптор сокета.
    \param buf Указатель на буфер в который будут прочитаны данные.
    \param len Размер буфера, т.е. максимальное кол-во байт для чтения.
    \param na Указатель на структуру net_addr_t получающую адресную информацию о локальных и удаленных портах и удаленных IP-адресах.
    \param flags Флаги, регулирующие функционирование (любая комбинация из NET_FLG_PEEK и NET_FLG_NON_BLOCKING).
    \return
    - при успехе: количество прочитанных байт
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo, код возврата 0 означает, что пир закрывает соединение.
*/
int nio_recv(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    /*
    Указатели в рамках этого модуля могут быть дальними или нет в зависимости от модели памяти используемой компилятором.
    Следующий код будет гарантировать, что дальний указатель передается в Sockets полностью нулевым,
    если смещение указателя равно нулю.
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
    Читает из сети с помощью сокетов и предназначен только для использования для DATAGRAM сокетов.
    Все датаграмм с IP-адресов и портов возвращают соответствующие значения в структуру net_addr_t в то время как другие отбрасываются.
    Нулевое значение для remote_host используется в качестве шаблона, чтобы получить с любого хоста и
    нулевое значение для remote_port используется в качестве шаблона, чтобы получить из любого порта.
    Локальный порт, local_port, не могут быть указаны как нулевые.
    В других отношениях nio_recv_from() ведет себя так же, как nio_recv().
    \param socket Дескриптор сокета.
    \param buf Указатель на буфер в который будут прочитаны данные.
    \param len Размер буфера, т.е. максимальное кол-во байт для чтения.
    \param na Указатель на структуру net_addr_t получающую адресную информацию о локальных и удаленных портах и удаленных IP-адресах.
    \param flags Флаги, регулирующие функционирование (любая комбинация из NET_FLG_PEEK и NET_FLG_NON_BLOCKING).
    \return
    - при успехе: количество прочитанных байт
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo, код возврата 0 означает, что собеседник закрывает соединение.
    .
    \note Обратите внимание на следующие аномалии: Если блокировка отключена,
    неудача с кодом ошибки ErrWouldBlock совершенно нормально и только означает, что данные не в настоящее время.
*/
int nio_recv_from(int socket, char *buf, u16 len, net_addr_t *na, u16 flags)
{
    x86regs_t r;
    /*
    Указатели в рамках этого модуля могут быть дальними или нет в зависимости от модели памяти используемой компилятором.
    Следующий код будет гарантировать, что дальний указатель передается в Sockets полностью нулевым,
    если смещение указателя равно нулю.
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
    Запись в сети с помощью сокетов.
    \param socket Дескриптор сокета.
    \param buf Указатель на буфер содержаший данные для передачи.
    \param len Размер буфера, т.е. кол-во байт для передачи.
    \param flags Флаги, регулирующие функционирование
            (любая комбинация из NET_FLG_OOB, NET_FLG_PUSH, NET_FLG_NON_BLOCKING, NET_FLG_BROADCAST, NET_FLG_MC_NOECHO).
    \return
    - при успехе: количество переданных байт
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
    .
    \note Кол-во байт фактически записанных при неблокирующем сокете, может быть меньше, чем длина \a len.
    В таком случае, передачу неотправленных данных нужно повторить, предпочтительно с некоторой задержкой.
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
    Запись в сети с помощью сетевых адресов (UDP только).
    \param socket Дескриптор сокета.
    \param buf Указатель на буфер содержаший данные для передачи.
    \param len Размер буфера, т.е. кол-во байт для передачи.
    \param na Указатель на структуру net_addr_t, содержащую локальный порт, чтобы написать и с удаленного порта и IP-адрес для записи.
    \param flags Флаги, регулирующие функционирование
            (любая комбинация из NET_FLG_NON_BLOCKING, NET_FLG_BROADCAST).
    \return
    - при успехе: количество переданных байт
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Устанавливает тревожный таймер???
    \param socket Дескриптор сокета.
    \param time Задержка таймера в миллисекундах.
    \param handler Дальний адрес сигнализации обратного вызова.
    Смотрите описание nio_set_async_notify() для формата функции обратного вызова.
    \param hint Аргумент, передаваемый функции обратного вызова.
    \return
    - при успехе: дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Устанавливает асинхронные уведомления (обратный вызов) для конкретного события.
    Другие функции CAPI может быть названа в обратного вызова, за исключением nio_resolve_name ),
    который можно назвать DOS.
    Обратного вызова не совместим с C аргумент в обход конвенции и некоторой осторожностью должны быть приняты.
    Некоторые манипуляции регистров процессора не требуется.
    Это может быть сделано путем ссылки на регистры процессора, такие, как _BX, или с помощью ассемблера инструкции.
    В обратного вызова, стек поставляется сокету и может быть весьма мал в зависимости от
    / с = параметр командной строки при загрузке Sockets.
    Сегмент стека, очевидно, не равна сегмента данных, которые могут вызвать проблемы при очень мелкий,
    мелкий или средний модель памяти используется.
    Самый простой способ решения этой проблемы заключается в использовании компактная, большой или огромный модели памяти.
    Другие варианты - использование DS = SS параметр компилятора или сделать стек переключиться на сегмент данных стека!.
    Если обратного вызова написан на С или C + +, _loadds модификатор может быть использован для установки сегмента данных,
    что и модуль, который разрушает DS используется для переменной аргумента.
    (Вот почему DS == SI на въезд для Sockets версии 1.04 и выше.)
    Альтернативный метод заключается в использовании аргумента передается nio_set_async_notify() в ES: DI,
    как указатель на структуру, которая доступна как с основной код и обратного вызова.
    Если DS не установлен на сегмент данных модуля, то функции в CAPI.C не работают: не использовать их в обратном.
    Обратного вызова, вероятно, будет выполняться в режиме прерывания без гарантии спускаемого для DOS.
    Не использовать любые функции, такие как риЬспаг () или Е (), в функции обратного вызова, которые могут вызвать DOS для вызова.
    Это хорошая практика программирования делать как можно меньше в обратный вызов.
    Установка флагов событий, которые вызывают операции в более стабильные времена рекомендуется.
    Функции обратного вызова не гнездятся.
    Функция обратного вызова не вызывается в то время как обратный вызов продолжается, даже если другие функции CAPI называются.
    Для смягчения проблемы в пунктах 2, 3 и 4 выше, обработчик приводится в CAPI.C который
    использует подсказку параметр для передачи адреса C-совместимый обработчик,
    с дымовой трубой, который является также C-совместимым.
    Этот обработчик назван nio_async_notify_handler.
    Пользователь обработчик с именем MyHandler ниже, называется в обычном порядке со стеком в 500 байт.
    Размер стека значение может быть установлено путем изменения HANDLER_STACK_SIZE постоянной в CAPI.C.
    \param socket Дескриптор сокета.
    \param event Событие которое может быть установлено из net_async_notify_route_t.
    \param handler Дальний адрес функции обратного вызова.
    \param hint Аргумент, передаваемый функции обратного вызова.
    \return
    - при успехе: указатель на предыдущий обработчик
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Разрешает IP-адрес из символического имени.
    \param zname Указатель на строку содержащую символическое имя.
    \param cname Указатель на буфер принимающий каноническое имя.
    \param cnamelen Размер буфера указывающего на каноническое имя.
    \return
    - при успехе: IP адрес
    - при ошибке: 0, с установкой кода ошибки в SocketsErrNo
*/
u32 nio_resolve_name(char *zname, char *cname, int cnamelen)
{
    x86regs_t r;
    /*
    Указатели в рамках этого модуля могут быть дальними или нет в зависимости от модели памяти используемой компилятором.
    Следующий код будет гарантировать, что дальний указатель передается в Sockets полностью нулевым,
    если смещение указателя равно нулю.
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
    Получает IP-адрес из десятичного адреса.
    \param zname Указатель на строку содержащую десятичный адрес с точкой.
    \return
    - при успехе: IP адрес
    - при ошибке: 0, с установкой кода ошибки в SocketsErrNo
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
    Устанавливает параметры сокета.
    \param socket Дескриптор сокета.
    \param level Уровень свойства. Это значение игнорируется.
    \param opt Устанавливаемое свойство (NET_OPT_NON_BLOCKING, NET_OPT_TIMEOUT, NET_OPT_WAIT_FLUSH).
    \param optval Значение свойства.
    \param len Размер значения свойства. Во всех случаях равно 4-м.
    \return
    - при успехе: глобальный дескриптор сокета
    - при ошибке: -1, с установкой кода ошибки в SocketsErrNo
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
    Включает сокет в группу многоадресной рассылки.
    \param groupaddr Групповой адрес по которому идет получение групповых дейтаграмм.
    \param ifaceaddr IP-адрес для использования интерфейса.
    Первый интерфейс который будет указан в SOCKET.CFG является интерфейсом по умолчанию в случае когда ifaceaddr == 0.
    \return
    - при успехе: 0
    - при ошибке: любое другое значение целого содержащее код ошибки.
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
    Исключает сокет из группы многоадресной рассылки.
    \param groupaddr Групповой адрес по которому идет получение групповых дейтаграмм.
    \param ifaceaddr IP-адрес для использования интерфейса.
    Первый интерфейс который будет указан в SOCKET.CFG является интерфейсом по умолчанию в случае когда ifaceaddr == 0.
    \return
    - при успехе: 0
    - при ошибке: любое другое значение целого содержащее код ошибки.
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
    Функция управления асинхронными интерфейсами.
    \param zname Указатель на имя интерфейса.
    \param fun Функция для выполнения ioctl_functions_t.
    \return
    - при успехе: >= 0
    - при ошибке: -1.
    .
    Функция IOCTL_GETSTATUS возвратит сочетания бит:
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
