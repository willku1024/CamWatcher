#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <pthread.h>
int socket_fd;
int client_num;

//pthread_mutex_t mutex;
pthread_cond_t cond;
//pthread_attr_t attr;
extern void init_socket(int port);
extern void start_listen(int max_lia);
extern void uninit_socket();




#endif // SERVER_H_INCLUDED
