#include "..\sio\sio.h"
#include <stdio.h>
//---------------------------------------------------------------------------------
int main(void)
{
    char sendcmd[2000];
    char recvcmd[2000];
    unsigned int i;

    sio_com_t   nport = SIO_COM1;

    for(i=0;i<256;i++) sendcmd[i] = (char)i;

    printf ("Start\n");

  
    sio_open(nport, SIO_BLOCK_MODE, 512, 512);

    sio_configure(nport, SIO_BPS_19200, SIO_PAR_NONE, SIO_DATA8, SIO_STOP1);

    sio_send(nport, (const char *)sendcmd, 256);

    sio_recv(nport, (char *)recvcmd, 5);

    i = 0;
    for (i = 0; i<6; i++)
    {
            printf("%c(%Xh)\t", recvcmd[i],(recvcmd[i]&0x00ff));
    }

    sio_close(nport);

    printf ("\n\nm_err = %d\n", sioerrno);
    printf ("End\n");
}
