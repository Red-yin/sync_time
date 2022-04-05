#ifndef UDP_IO_H__
#define UDP_IO_H__

#include "my_epoll.h"
#include "msg_queue.h"
class UdpIO
{
private:
	int server_fd = -1;
	Epoll *epoll;
	MsgQueue *data_queue; 
	char m_group_ip[32];
	int m_port;
public:
    UdpIO(const char *group_ip, int port, Epoll *ep, MsgQueue *q);
    ~UdpIO();
	int send(const char *ip, int port, void *data, unsigned len);
	int send(struct sockaddr_in *addr, void *data, unsigned len);
	int send(void *data, unsigned len);
	static void recv(int fd, void *param);
};


#endif
