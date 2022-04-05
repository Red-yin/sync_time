#include "tcp_io.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>
extern "C"{
#include "slog.h"
};



TcpIO::TcpIO(int port, Epoll *ep, MsgQueue *q):
	server_fd(-1), epoll(ep), data_queue(q)
{
#if 1
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	des_addr.sin_port = htons(port);
	if (bind(server_fd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0)
	{
		err("socket bind failed\n");
		return;
	}
	if (listen(server_fd, 256) == -1)
	{
		err("Listen error:%s", strerror(errno));
		return;
	}

	Epoll::FdInfo info;
	memset(&info, 0, sizeof(info));
	info.type = Epoll::SERVER;
	epoll->add(server_fd, tcp_accept, (void *)this, &info);
#endif
}

TcpIO::~TcpIO()
{
	if(server_fd > 0){
		close(server_fd);
	}
}

int TcpIO::send(const char *ip, int port, const void *data, unsigned len)
{
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &des_addr.sin_addr);
	//des_addr.sin_addr.s_addr = inet_addr(ip);
	des_addr.sin_port = htons(port);

	return send(&des_addr, data, len);
}

int TcpIO::send(struct sockaddr_in *addr, const void *data, unsigned len)
{
	//1.connect; 2.sendto; 3.save fd in epoll;
	int fd = epoll->find_fd(addr);
	if(fd < 0){
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if(connect(fd, (struct sockaddr *)addr, sizeof(struct sockaddr)) < 0){
			err("connect failed: %s", strerror(errno));
			return -1;
		}

		Epoll::FdInfo info;
		memset(&info, 0, sizeof(info));
		info.addr = *addr;
		info.type = Epoll::CLIENT;
		epoll->add(fd, recv, (void *)this, &info);
	}
	dbg_t("data: %s, len: %d", data, len);
	return sendto(fd, data, len, 0, (struct sockaddr *)addr, sizeof(struct sockaddr));
}

int TcpIO::over(const char *ip, int port)
{
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr.s_addr = inet_addr(ip);
	des_addr.sin_port = htons(port);

	return over(&des_addr);
}

int TcpIO::over(struct sockaddr_in *addr)
{
	int fd = epoll->find_fd(addr);
	if(fd < 0){
		return -1;
	}

	over(fd);
	return 0;
}

int TcpIO::over(int fd)
{
	epoll->del(fd);
	close(fd);
	return 0;
}

#if 0
void TcpIO::wait(void *param)
{
	TcpIO *t = (TcpIO *)param;
	dbg_t("waiting...");
	while(1){
		int conn_fd = -1;
		struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
		bzero(&client_addr, sizeof(client_addr));
		int client_len = sizeof(client_addr);
		if ((conn_fd = accept(t->server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len)) == -1) {
			err("accept socket error: %s", strerror(errno));
			continue;
		}
		dbg_t("accept: %d", conn_fd);
		t->epoll->add(conn_fd, recv, param);
	}
}

void TcpIO::wait_nb()
{
	t = std::move(std::thread(wait, this));
}
#endif

void TcpIO::tcp_accept(int fd, void *param)
{
	TcpIO *t = (TcpIO *)param;
	int conn_fd = -1;
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	bzero(&client_addr, sizeof(client_addr));
	int client_len = sizeof(client_addr);
	if ((conn_fd = accept(t->server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len)) == -1) {
		printf("accept socket error: %s\n\a", strerror(errno));
		return;
	}
	dbg_t("accept: %d", conn_fd);
	//recv(conn_fd);
	t->epoll->add(conn_fd, recv, param);
}

void TcpIO::recv(int fd, void *param)
{
	TcpIO *t = (TcpIO *)param;
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	bzero(&client_addr, sizeof(client_addr));
	int client_len = sizeof(client_addr);
	char *data = (char *)calloc(1, 32);
	int buf_len = 32;
	//系统调用recv系列函数在收到断开TCP连接的信息时会返回0；
	//另外，如果给recv系列函数传递参数data_len=0，也会收到返回0；
	//因此，请保证调用recv系列函数时传递的参数data_len > 0;
	int n = recvfrom(fd, (char *)data, buf_len, 0, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
	dbg_t("epoll recv data from %d, data len: %d, [%s]", fd, n, data);
	if(n == 0){
		free(data);
		close(fd);
		if(t && t->epoll)
			t->epoll->del(fd);
		dbg_t("%d closed", fd);
	}else if(n < 0){
		free(data);
		err("%s", strerror(errno));
	}else{
		t->data_queue->put(data);
	}
}

in_addr TcpIO::self_ip()
{
	struct ifconf ifc;
	struct ifreq buf[16];
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t)buf;
	ioctl(server_fd, SIOCGIFCONF, (char *)&ifc);
	int intr = ifc.ifc_len / sizeof(struct ifreq);
	while (intr-- > 0 && ioctl(server_fd, SIOCGIFADDR, (char *)&buf[intr]));

	return ((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr;
}
