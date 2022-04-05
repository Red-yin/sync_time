#include "msg_io.h"
#include "msg_queue.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

extern "C"{
#include "slog.h"
};

#define TEST 1
enum transfer_method{
    TCP,
    UDP_UNICAST,
    UDP_MULTICAST
};

#define PORT 12345

typedef struct transfer_msg_info{
    enum transfer_method type;
    int fd;
    void *data;
    unsigned data_len;
    struct sockaddr_in addr;
}transfer_msg_info_t;

typedef struct msg_transfer{
    transfer_msg_info_t *msg;
    MsgQueue *mq;
}msg_transfer_t;

MsgIO::MsgIO(MsgIOMode mode)
{
    switch(mode){
        case MASTER:
            init_master();
            break;
        case CLIENT:
            init_client();
            break;
    }
}

MsgIO::~MsgIO()
{
    destory();
}

void MsgIO::init_master()
{
    mq = new MsgQueue();
    tp = new ThreadPool(4, 10);
    udp_input = thread(recv_multicast_msg, this);
    tcp_input = thread(recv_unicast_msg, this);

}

void MsgIO::init_client()
{
    udp_output = thread(send_msg, this);
    mq = new MsgQueue();
}

void MsgIO::destory()
{
    //TODO:线程如何退出，线程资源如何回收
}

void MsgIO::send_msg(void *arg)
{
    transfer_msg_info_t *msg_info = (transfer_msg_info_t *)arg;
    while(1){
        sendto(msg_info->fd, msg_info->data, msg_info->data_len, 0, (struct sockaddr *)&msg_info->addr, sizeof(msg_info->addr));
        sleep(1);
    }
}

void send_multicast_msg(transfer_msg_info_t *arg)
{

}

ssize_t send_upd_msg(transfer_msg_info_t *arg)
{
    struct sockaddr_in des_addr;
    des_addr = arg->addr;

    return sendto(arg->fd, arg->data, arg->data_len, 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

void send_tcp_msg(transfer_msg_info_t *arg)
{

}

void MsgIO::recv_multicast_msg(void *arg)
{
    MsgIO *msg = (MsgIO *)arg;
    int multicast_fd = msg->init_multicast(PORT);
    while (1)
    {
        transfer_msg_info_t *client_info = (transfer_msg_info_t *)calloc(1, sizeof(transfer_msg_info_t));
        if(client_info == NULL){
            printf("transfer_msg_info_t calloc failed");
            usleep(1000*100);
            continue;
        }
 
        char *buf = (char *)calloc(1, 32);
        socklen_t client_len = sizeof(struct sockaddr_in);
        int ret = recvfrom(multicast_fd, buf, 32, 0, (struct sockaddr *)&client_info->addr, &client_len);
        if(ret > 0){
            client_info->data_len = ret;
            client_info->data = (void *)buf;
            msg->mq->put(client_info);
        }
    #ifdef TEST
        printf("udp recv data: ip: %d, %s", (int)client_info->addr.sin_addr.s_addr, buf);
    #endif
    }
}

void *MsgIO::tcp_recv(void *arg)
{
    msg_transfer_t *param = (msg_transfer_t *)arg;
    transfer_msg_info_t *client_info = param->msg;
    char *buf = (char *)calloc(1, 32);
    int ret = recv(client_info->fd, (void *)buf, 32, 0);
    if(ret < 0){
        printf("client fd closed");
        return NULL;
    }
    client_info->data = (void *)buf;
    client_info->data_len = ret;
    param->mq->put(client_info);
    free(param);
    
    #ifdef TEST
    printf("tcp recv data: fd: %d, %s", client_info->fd, buf);
    free(buf);
    free(client_info);
    #endif
    return NULL;
}

void MsgIO::recv_unicast_msg(void *arg)
{
    MsgIO *msg = (MsgIO *)arg;
    int unicast_fd = msg->init_unicast(PORT);
    char buf[32];
    while (1)
    {
        msg_transfer_t *param = (msg_transfer_t *)calloc(1, sizeof(msg_transfer_t));
        if(param == NULL){
            printf("msg_transfer calloc failed");
            usleep(1000*100);
            continue;
        }
        transfer_msg_info_t *client_info = (transfer_msg_info_t *)calloc(1, sizeof(transfer_msg_info_t));
        if(client_info == NULL){
            printf("transfer_msg_info_t calloc failed");
            usleep(1000*100);
            continue;
        }
        //struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(struct sockaddr_in);
        int conn_fd;
        if ((conn_fd = accept(unicast_fd, (struct sockaddr *)&client_info->addr, &client_len)) == -1) {
            printf("accept socket error: %s\n\a", strerror(errno));
            continue;
        }
        client_info->fd = conn_fd;
        param->mq = msg->mq;
        param->msg = client_info;
        msg->tp->add_job(tcp_recv, param);
    }
}

int MsgIO::init_multicast(int port)
{
    int multicast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in des_addr;
    bzero(&des_addr, sizeof(des_addr));
    des_addr.sin_family = AF_INET;
    des_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    des_addr.sin_port = htons(port);
    /* 设置通讯方式广播，即本程序发送的一个消息，网络上所有的主机均可以收到 */
    setsockopt(multicast_fd, SOL_SOCKET, SO_BROADCAST, &multicast_fd, sizeof(multicast_fd));
    if (bind(multicast_fd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0)
    {
        err("socket bind failed\n");
    }
    return multicast_fd;
}

int MsgIO::init_unicast(int port)
{
    int unicast_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in des_addr;
    bzero(&des_addr, sizeof(des_addr));
    des_addr.sin_family = AF_INET;
    des_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    des_addr.sin_port = htons(port);
    if (bind(unicast_fd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0)
    {
        err("socket bind failed\n");
    }
    if (listen(unicast_fd, 256) == -1)
    {
        err("Listen error:%s", strerror(errno));
        exit(1);
    }
    return unicast_fd;
}
