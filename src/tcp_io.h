#ifndef TCP_IO_H__
#define TCP_IO_H__

#include "my_epoll.h"
#include "msg_queue.h"
#include <arpa/inet.h>

class TcpIO
{
public:
    TcpIO(int port, Epoll *ep, MsgQueue *q);
    ~TcpIO();
	int send(const char *ip, int port, const void *data, unsigned len);
	int send(struct sockaddr_in *addr, const void *data, unsigned len);
	int over(const char *ip, int port);
	int over(struct sockaddr_in *addr);
	int over(int fd);
	static void recv(int fd, void *param);
	static void tcp_accept(int fd, void *param);
	in_addr self_ip();
#if 0
	static void wait(void *param);
	void wait_nb();
#endif
    
	int server_fd; 
private:
	Epoll *epoll;
	MsgQueue *data_queue;
};

#endif
