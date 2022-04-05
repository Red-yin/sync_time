#include "icmp_io.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
extern "C"{
#include "slog.h"
};


IcmpIO::IcmpIO(Epoll *ep): epoll(ep)
{
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(fd < 0){
		err("icmp socket init failed: %s", strerror(errno));
		return;
	}
	epoll->add(fd, recv, (void *)this);
}
		
IcmpIO::~IcmpIO()
{
	if(fd > 0){
		close(fd);
	}
}
		
void IcmpIO::recv(int fd, void *param)
{
	IcmpIO *i = (IcmpIO *)param;
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	bzero(&client_addr, sizeof(client_addr));
	int client_len = sizeof(client_addr);
	char data[512] = {0};
	int buf_len = sizeof(data);
	int n = recvfrom(fd, (char *)data, buf_len, 0, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
	dbg_t("icmp recv from %d data len %d", fd, n);
	if(n == 0){
		close(fd);
		if(i && i->epoll)
			i->epoll->del(fd);
		dbg_t("icmp %d closed", fd);
	}else if(n < 0){
		err("%s", strerror(errno));
	}else{
		i->icmp_printf((void *)data, n);
	}
}

void IcmpIO::icmp_printf(void *data, int len)
{
	if(len <= 0){
		return;
	}
	char a = 0x01;
	char *d = (char *)data;
	for(int i = 0; i < len; i++){
		printf("%d:", i);
		for(int j = 7; j >= 0; j--){
			char b = a << j;
			char r = ((unsigned char)(d[i] & b))>>j;
			printf("%d", r);
		}
		printf("\t");
	}
	printf("\n");
}
