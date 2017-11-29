#include "server.h"
#include "write_log.h"
#include "print.h"
#include "device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//初始化了一个MUTEX锁
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void init_socket(int port)
{
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    ERR_PUTS("socket");
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&addr.sin_zero,0,sizeof(addr.sin_zero));

    int val = 1;
    setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
    bind(socket_fd,(struct sockaddr*)(&addr),sizeof(addr));
    ERR_PUTS("bind");

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    client_num = 0;
}
void thread_exit(long fd)
{


    pthread_mutex_lock(&mutex);
    client_num--;
    pthread_mutex_unlock(&mutex);

    printf("client offline,current nums:%d\n",client_num);

    close(fd);
    pthread_exit(NULL);
}

void * do_service(void *arg)
{
    long fd = (long)arg;
    //int res = write(fd,"hello",5);
    char head[4096];
    sprintf(head,
            "HTTP/1.0 200 OK\r\n"    //状态行
            "Connection: Keep-Alive\r\n"
            "Server: Network camera\r\n"
            "Cache-Control: no-cache,no-store,must-revalidate,pre-check=0,max-age=0\r\n"
            "Pragma: no-cache\r\n"
            "Content-Type: multipart/x-mixed-replace;boundary=KK\r\n");
    write(fd,head,strlen(head));

    while(1)
    {
        //usleep(35);
        //消息报头
        sprintf(head,
                "\r\n--KK\r\n"
                "Content-Type: image/jpeg\n"
                "Content-Length: %ld\n\n",buffer[okindex].length+400);
        if(write(fd,head,strlen(head)) != strlen(head)) break;
        //get_frame();
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond,&mutex);
        print_picture(fd,tmp_buf,buffer[okindex].length);
        pthread_mutex_unlock(&mutex);
    }
    thread_exit(fd);
    return 0;

}
void * do_frame(void * arg)
{
    while(1)
    {
        if(client_num >= 1)
        {
            //usleep(35);
            get_frame();
            pthread_mutex_lock(&mutex);
            memcpy(tmp_buf,buffer[okindex].start,buffer[okindex].length);
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            break;
        }
    }
    cam_off();
    pthread_exit(NULL);
}

void start_listen(int max_lia)
{
    listen(socket_fd,max_lia);

    pthread_t th;
    int fd;

    printf("!!! server start accepting !!!\n");
    write_log(1,"start_listen()","!!! server start accepting !!!");
    while(1)
    {

        struct sockaddr_in clientaddr_in;
        socklen_t len = sizeof(clientaddr_in);
        memset(&clientaddr_in,0,sizeof(clientaddr_in));
        fd = accept(socket_fd,(struct sockaddr*)&clientaddr_in,&len);

        if(fd < 0)
        {
            fprintf(stderr,"accept error: %s\n",strerror(errno));
            exit(1);
        }
        else
        {
            char logbuf[100] = {0};


            pthread_t cth;
            int err = pthread_create(&cth,NULL,do_service,(void *)fd);

            pthread_mutex_lock(&mutex);
            client_num++;
            pthread_mutex_unlock(&mutex);

            if(client_num>6)
            {
                write_log(2,"start_listen()","server is busy.connect failed.");
                continue;
            }
            else
            {
                sprintf(logbuf,"remote :%s:%d connect server.", inet_ntoa(clientaddr_in.sin_addr),
                        ntohs(clientaddr_in.sin_port));
                write_log(1,"start_listen()",logbuf);

            }

            if(client_num==1)
            {
                cam_on();
                int err = pthread_create(&th,NULL,do_frame,NULL);
                if(err < 0)
                {
                    fprintf(stderr,"create frame pthread error: %s \n",strerror(errno));
                }
            }


            if(err < 0)
            {
                fprintf(stderr,"create client pthread error: %s\n",strerror(errno));
            }
            printf("client num: %d\n",client_num);
        }

    }

}

void uninit_socket()
{
    close(socket_fd);

}







