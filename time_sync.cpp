#include "time_sync.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
//#include <thread>

#define MASTER_PORT 7655
#define ALIVE_TIME 60000

TimeSync::TimeSync()
{
	mode = SLAVE;
	struct sockaddr_in des_addr;

	recv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	//des_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); //广播地址
	des_addr.sin_addr.s_addr = inet_addr(INADDR_ANY); //广播地址
	des_addr.sin_port = htons(MASTER_PORT);
	 /* 设置通讯方式广播，即本程序发送的一个消息，网络上所有的主机均可以收到 */
    setsockopt(recv_sockfd,SOL_SOCKET,SO_BROADCAST,&recv_sockfd,sizeof(recv_sockfd));
	if (bind(recv_sockfd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0){
		printf("socket bind failed\n");
	}

	//创建一个线程接收消息以及处理消息
	//t = std::thread(recv, this);
	//pthread_create(&pt, NULL, recv, (void *)this);
	start();
}

void TimeSync::run()
{
	MsgContent_T content = {};
	char recvbuf[16] = {0};
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	socklen_t client_len;
	int n = 0;
	while(1){
		client_len = sizeof(client_addr);
		n = recvfrom(recv_sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&client_addr, &client_len);
		//t->handle(t->mode, &content);
		sendto(recv_sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client_addr, client_len);  //发送信息给client，注意使用了client_addr结构体指针
	}
}

void TimeSync::send(MsgType type, RunMode_E mode)
{
	switch(mode){
		case MASTER:{
			switch(type){
				case BOARDCAST:
					//发送时间戳广播
					break;
				case BOARDCAST_RESPONSE:
					//不响应
					break;
				case WAKEUP:
					//发送给特定IP唤醒事件
					break;
				case BET:
					//不响应
					break;
			}
			break;
		}
		case SLAVE:{
			switch(type){
				case BOARDCAST:
					//不响应
					break;
				case BOARDCAST_RESPONSE:
					//发送本机IP和接收到的时间戳
					break;
				case WAKEUP:
					//被唤醒，把事件发送给MASTER
					break;
				case BET:
					//发送BET广播
					break;
			}
			break;
		}
		default:
			break;
	}
}

void TimeSync::handle(RunMode_E mode, MsgContent &content)
{
	switch(mode){
		case MASTER:{
			switch(content.type){
				case BOARDCAST:
					//检查接收内容IP是否和本机一致，否则报错
					break;
				case BOARDCAST_RESPONSE:
					//计算content->ip对应设备与本机的延迟，并保存到队列
					break;
				case WAKEUP:
					//比对最早唤醒的设备，更新潜在响应唤醒事件设备信息
					break;
				case BET:
					//发送错误广播，告知本机为MASTER模式
					break;
			}
			break;
		}
		case SLAVE:{
			switch(content.type){
				case BOARDCAST:
					//更新系统时间，立即回复BOARDCAST_RESPONSE给content->ip
					break;
				case BOARDCAST_RESPONSE:
					//不响应
					break;
				case WAKEUP:
					//响应唤醒事件
					break;
				case BET:
					//发送BET广播
					break;
			}
			break;
		}
		default:
			break;
	}
}

