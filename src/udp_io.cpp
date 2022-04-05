#include "udp_io.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
extern "C"{
#include "slog.h"
};

UdpIO::UdpIO(const char *group_ip, int port, Epoll *ep, MsgQueue *q): epoll(ep), data_queue(q)
{
	memset(m_group_ip, 0, sizeof(m_group_ip));
	strcpy(m_group_ip, group_ip);
	m_port = port;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);

	//设置多播生命值，默认为1，只在本路由器下传播
	unsigned char ttl = 1;
	setsockopt(server_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
	//设置是否允许数据回送本地回环接口，默认允许，设置为0将禁止回送
	unsigned char loop = 0;
	setsockopt(server_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	//加入一个多播组
	struct ip_mreq ipmr;
	ipmr.imr_interface.s_addr = htonl(INADDR_ANY);
	ipmr.imr_multiaddr.s_addr = inet_addr(group_ip);
	//inet_pton(AF_INET, group_ip, &ipmr.imr_multiaddr); 
	setsockopt(server_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ipmr, sizeof(ipmr));

    struct sockaddr_in des_addr;
    bzero(&des_addr, sizeof(des_addr));
    des_addr.sin_family = AF_INET;
	//inet_pton(AF_INET, group_ip, &des_addr.sin_addr);
    des_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    des_addr.sin_port = htons(port);
    /* 设置通讯方式广播，即本程序发送的一个消息，网络上所有的主机均可以收到 */
    //setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &server_fd, sizeof(server_fd));
    if (bind(server_fd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0)
    {
        err("socket bind failed: %s", strerror(errno));
    }
	epoll->add(server_fd, recv, (void *)this);
}

UdpIO::~UdpIO()
{
	if(server_fd > 0){
		close(server_fd);
	}
}

int UdpIO::send(const char *ip, int port, void *data, unsigned len)
{
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &des_addr.sin_addr);
	des_addr.sin_port = htons(port);

	return send(&des_addr, data, len);
}

int UdpIO::send(struct sockaddr_in *addr, void *data, unsigned len)
{
	return sendto(server_fd, data, len, 0, (struct sockaddr *)addr, sizeof(struct sockaddr));
}

int UdpIO::send(void *data, unsigned len)
{
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	inet_pton(AF_INET, m_group_ip, &des_addr.sin_addr);
	des_addr.sin_port = htons(m_port);
	dbg_t("sending ip: %s, port: %d", m_group_ip, m_port);
	return sendto(server_fd, data, len, 0, (struct sockaddr *)&des_addr, sizeof(struct sockaddr));
}

void UdpIO::recv(int fd, void *param)
{
	UdpIO *u = (UdpIO *)param;
	
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	bzero(&client_addr, sizeof(client_addr));
	int client_len = sizeof(client_addr);
	char *data = (char *)calloc(1, 32);
	int buf_len = 32;
	int n = recvfrom(fd, (char *)data, buf_len, 0, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
	dbg_t("udp recv from %d data len %d", fd, n);
	if(n == 0){
		free(data);
		close(fd);
		if(u && u->epoll)
			u->epoll->del(fd);
		dbg_t("%d closed", fd);
	}else if(n < 0){
		free(data);
		err("%s", strerror(errno));
	}else{
		u->data_queue->put(data);
	}
}
