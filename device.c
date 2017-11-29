#include "device.h"
#include "print.h"
#include "write_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define FLUSH_NUM 1
void suc_err(int res, char* str)
{
    if (res < 0)
    {
        fprintf(stderr, "%s error: %s\n",str,strerror(errno));
        exit(1);
    }

}
void init_fmt()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//数据流的类型
    fmt.fmt.pix.width = 320;//图像的宽度
    fmt.fmt.pix.height = 240;//图像的高度
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//彩色空间
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    ioctl(camera_fd, VIDIOC_S_FMT, &fmt);
    ERR_PUTS("format");
}
void init_mmap()
{

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = FLUSH_NUM;//缓存数量
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
     ioctl(camera_fd, VIDIOC_REQBUFS, &req);
    ERR_PUTS( "Req_bufs");

    buffer = calloc(req.count, sizeof(Videobuf));
    struct v4l2_buffer buf;
    for (bufs_num = 0; bufs_num < req.count; bufs_num++)
    {
        memset(&buf, 0, sizeof(buf));
        buf.index = bufs_num;//设置缓存索引号
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.field = V4L2_FIELD_INTERLACED;
        buf.memory = V4L2_MEMORY_MMAP;
        //读取缓存信息
         ioctl(camera_fd, VIDIOC_QUERYBUF, &buf);
        ERR_PUTS( "Query_buf");
        //设置缓存大小
        buffer[bufs_num].length = buf.length;
        //在堆空间中动态分配二级缓存空间
        tmp_buf = (unsigned char*)calloc(buffer[okindex].length, sizeof(char));
        //将设备文件的地址映射到用户空间的物理地址
        buffer[bufs_num].start = mmap(NULL,
                                      buf.length,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED,
                                      camera_fd,
                                      buf.m.offset);
        if (buffer[bufs_num].start == MAP_FAILED)
            write_log(3, "init_mmap()","mmap failed.");
        else
            write_log(1, "init_mmap()","mmap success.");

    }
}


int get_dev_info()
{
    //获取当前设备的属性
    struct v4l2_capability cap;
    ioctl(camera_fd, VIDIOC_QUERYCAP, &cap);
    ERR_PUTS( "get cap");
    //获取当前设备的输出格式
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     ioctl(camera_fd, VIDIOC_G_FMT, &fmt);
    ERR_PUTS( "get format");
    //获取当前设备的帧率
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     ioctl(camera_fd, VIDIOC_G_PARM, &parm);
    ERR_PUTS( "get parm");
    //打印输出设备信息：
    printf("----------------dev_info---------------\n");
    printf("driver:	%s\n", cap.driver);
    printf("card:	%s\n", cap.card); //摄像头的设备名
    printf("bus:	%s\n", cap.bus_info);
    printf("width:	%d\n", fmt.fmt.pix.width); //当前的图像输出宽度
    printf("height:	%d\n", fmt.fmt.pix.height); //当前的图像输出高度
    printf("FPS:	%d\n", parm.parm.capture.timeperframe.denominator);
    printf("------------------end------------------\n");

    return 0;
}

int cam_on()
{
    //通过v4l2打开摄像头
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(camera_fd, VIDIOC_STREAMON, &type)<0)
    {
        ERR_PUTS( "camera on");
        write_log(3,"cam_on()","camera open failed");
    }

    //进行一次缓存刷新
    struct v4l2_buffer buf;
    int i;
    for (i = 0; i < bufs_num; i++)
    {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if(ioctl(camera_fd, VIDIOC_QBUF, &buf)<0)
        {
            ERR_PUTS( "Q_buf_init");
            write_log(3,"cam_on()","Q_buf_init failed");

        }

    }
    return 0;
}

int cam_off()
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(camera_fd, VIDIOC_STREAMOFF, &type);
    on_off = 0;
    ERR_PUTS( "close stream");
    write_log(2,"cam_off","camera has been closed");
    return 0;
}

//捕获图像
int get_frame(void)
{
    struct v4l2_buffer buf;

    counter++;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    fd_set	readfds;
    FD_ZERO(&readfds);
    FD_SET(camera_fd, &readfds);
    struct timeval tv;//设置设备响应时间
    tv.tv_sec = 1;//秒
    tv.tv_usec = 0;//微秒
    while (select(camera_fd + 1, &readfds, NULL, NULL, &tv) <= 0)
    {
        fprintf(stderr, "camera busy,Dq_buf time out\n");
        FD_ZERO(&readfds);
        FD_SET(camera_fd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
    }
     ioctl(camera_fd, VIDIOC_DQBUF, &buf);
    ERR_PUTS( "Dq_buf");

    //buf.index表示已经刷新好的可用的缓存索引号
    okindex = buf.index;
    //更新缓存已用大小
    buffer[okindex].length = buf.bytesused;
    //第n次捕获图片:(第n回刷新整个缓存队列-第n个缓存被刷新)
    ////printf("Image_%03d:(%d-%d)\n",counter,counter / bufs_num,okindex);


    //把图像放入缓存队列中(入列)
     ioctl(camera_fd, VIDIOC_QBUF, &buf);
    ERR_PUTS( "Q_buf");

    return 0;
}


void install_dev()
{
    init_fmt();
    init_mmap();
    write_log(1,"install_dev()","fmt & mmap has been inited.");

}

void uninstall_dev()
{

    int i;
    for (i = 0; i < bufs_num; ++i)
    {
         munmap(buffer[i].start, buffer[i].length);
        ERR_PUTS( "munmap");
    }
    free(buffer);
    free(tmp_buf);
    close(camera_fd);
}


