#include "device.h"
#include "server.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void  sig_handler(int signo)
{
    if(signo == SIGPIPE)
    {
        printf("Recv SIGPIPE From Browser.Don't Worry.\n");
    }
}
void str_err(const char* name)
{
    fprintf(stderr,"%s: %s\n",name,strerror(errno));
    exit(1);
}
int main(int argc,char** argv)
{
    if(argc < 3)
    {
        fprintf(stderr,"Usage: %s [dev] [port]\n",argv[0]);
        exit(1);
    }

    signal(SIGPIPE,sig_handler);
    camera_fd = open(argv[1],O_RDWR|O_NONBLOCK);


    install_dev();
    //cam_on();

    init_socket(atoi(argv[2]));
    start_listen(10);
    uninit_socket();
    //cam_off();
    uninstall_dev();

    return 0;
}
