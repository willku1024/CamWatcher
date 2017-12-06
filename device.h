#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

#include <sys/types.h>
#include <errno.h>

#define ERR_PUTS(m)                        \
            do                             \
            {                              \
                if(errno)                  \
                    perror(m);             \
                                           \
            }while(0)


int camera_fd;
typedef struct{
    void* start;
    size_t length;
}Videobuf;

Videobuf* buffer;

//void suc_err(int res, char* str);
int bufs_num;

int counter;

int okindex;

unsigned char* tmp_buf;

int on_off;

extern void install_dev();

extern void uninstall_dev();

extern int get_frame();

extern int cam_on();

extern int cam_off();


#endif // DEVICE_H_INCLUDED
