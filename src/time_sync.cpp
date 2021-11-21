#include "time_sync.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <stdlib.h>

extern "C"{
	#include "slog.h"
};
//#include <thread>

#define MASTER_PORT 7655
#define ALIVE_TIME 1000
#define WAKEUP_DELAY 200

TimeSync::TimeSync()
{
	mode = SLAVE;
	dbg_t("mode: SLAVE");
	status = NORMAL;
	mtimer = new CTimer();
	timer_fd = mtimer->addTask(2*ALIVE_TIME, time_todo, this, 0);
	memset(&master_addr, 0, sizeof(master_addr));

	timeMapList = map_list_init(10);

	struct sockaddr_in des_addr;

	recv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	//des_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); //广播地址
	des_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	des_addr.sin_port = htons(MASTER_PORT);
	 /* 设置通讯方式广播，即本程序发送的一个消息，网络上所有的主机均可以收到 */
    setsockopt(recv_sockfd,SOL_SOCKET,SO_BROADCAST,&recv_sockfd,sizeof(recv_sockfd));
	if (bind(recv_sockfd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0){
		err("socket bind failed\n");
	}

	//创建一个线程接收消息以及处理消息
	//t = std::thread(recv, this);
	start();
}

TimeSync::~TimeSync()
{
	map_list_destory(&timeMapList);
	if(wakeup){
		free(wakeup);
	}
	delete mtimer;
}

void TimeSync::run()
{
	MsgContent_T content = {};
	char recvbuf[32] = {0};
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	socklen_t client_len;
	int n = 0;
	while(1){
		client_len = sizeof(client_addr);
		n = recvfrom(recv_sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&client_addr, &client_len);
		if(n <= 0){
			war("recv %d byte from %d", n, recv_sockfd);
			continue;
		}
		dbg_t("recv len: %d, data len: %d", n, (int)recvbuf[0]);
		content = *(MsgContent_T *)recvbuf;
		MsgContent_T & c = content;
		handle(mode, c);
		//sendto(recv_sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&client_addr, client_len);  //发送信息给client，注意使用了client_addr结构体指针
	}
}

long TimeSync::get_current_timestamp()
{
	struct timeval tv;
    gettimeofday(&tv, NULL);
	long cur_time = tv.tv_sec * 1000000 + tv.tv_usec;
	return cur_time;
}

in_addr TimeSync::self_ip()
{
	struct ifconf ifc;
	struct ifreq buf[16];
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = (caddr_t)buf;
	ioctl(recv_sockfd, SIOCGIFCONF, (char *)&ifc);
	int intr = ifc.ifc_len / sizeof(struct ifreq);
	while (intr-- > 0 && ioctl(recv_sockfd, SIOCGIFADDR, (char *)&buf[intr]));

	return ((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr;
}

int TimeSync::wakeup_response(pTaskContent task, void *param)
{
	TimeSync *t = (TimeSync *)param;
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr = t->wakeup->s_addr;
	des_addr.sin_port = htons(MASTER_PORT);
	MsgContent_T sendbuf;
	memset(&sendbuf, 0, sizeof(sendbuf));
	sendbuf.type = WAKEUP;

	MsgContent_T &c = sendbuf;
	dbg_t("native send:");
	t->print_TimesyncProtocol(c);
	sendto(t->recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));

	memset(t->wakeup, 0, sizeof(WakeupManage_T));
	return 0;
}

int TimeSync::time_todo(pTaskContent task, void *param)
{
	TimeSync *t = (TimeSync *)param;
	dbg_t("timer fd: %d", t->timer_fd);
#if 0
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); //广播地址
	des_addr.sin_port = htons(MASTER_PORT);
	MsgContent_T sendbuf;
	memset(&sendbuf, 0, sizeof(sendbuf));
#endif
	switch(t->mode){
		case MASTER:{
			//发送广播
#if 0
			sendbuf.type = BOARDCAST;
			sendbuf.len = sizeof(sendbuf) - 1;
			//time_t cur_time = time(NULL);
			long cur_time = t->get_current_timestamp();
			memcpy(sendbuf.timestamp, &cur_time, sizeof(sendbuf.timestamp));
			sendbuf.s_addr = t->self_ip();
			dbg_t("native send:");
			MsgContent_T &c = sendbuf;
			t->print_TimesyncProtocol(c);
			sendto(t->recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else			
			long cur_time = t->get_current_timestamp();
			t->sendBoardcast(BOARDCAST, cur_time);
#endif
			break;
		}
		case SLAVE:{
			switch(t->status){
				case NORMAL:{
					//获取ip
					sendbuf.s_addr = t->self_ip();
					//memcpy(&sendbuf.s_addr, &((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr.s_addr, sizeof(sendbuf.s_addr));
					t->status = WAIT_FOR_BET_RESULT;
					t->timer_fd = t->mtimer->addTask(ALIVE_TIME/2, time_todo, param, 0);
					//发起竞选
#if 0
					sendbuf.type = BET;
					sendbuf.len = sizeof(sendbuf) - 1;
					MsgContent_T &c = sendbuf;
					dbg_t("native send:");
					t->print_TimesyncProtocol(c);
					sendto(t->recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else
					t->sendBoardcast(BET, 0l);
#endif
					break;
				}
				case WAIT_FOR_BET_RESULT:{
					t->mode = MASTER;
					dbg_t("mode: MASTER");
					t->timer_fd = t->mtimer->addTask(ALIVE_TIME, time_todo, param, 1);
					t->wakeup = (WakeupManage_T *)calloc(1, sizeof(WakeupManage_T));
					//发送广播
#if 0
					sendbuf.type = BOARDCAST;
					sendbuf.len = sizeof(sendbuf) - 1;
					long cur_time = t->get_current_timestamp();
					dbg_t("cur time : %ld, size : %d", cur_time, sizeof(long));
					memcpy(sendbuf.timestamp, &cur_time, sizeof(sendbuf.timestamp));
					sendbuf.s_addr = t->self_ip();
					dbg_t("native send:");
					MsgContent_T &c = sendbuf;
					t->print_TimesyncProtocol(c);
					sendto(t->recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else
					long cur_time = t->get_current_timestamp();
					t->sendBoardcast(BOARDCAST, cur_time);
#endif
					t->master_addr = sendbuf.s_addr;
					break;
				}
			}
			break;
		}
	}
}

int TimeSync::bet(MsgContent_T &content)
{
	if(content.s_addr.s_addr > self_ip()){
		//如果赢得竞选，发送竞选广播
		sendBoardcast(BET, 0l);
	}else{
		//如果输了竞选，进入从模式初始化状态
		timer_fd = mtimer->addTask(2*ALIVE_TIME, time_todo, this, 0);
	}
}

ssize_t TimeSync::sendBoardcast(RecvMsgType_E type, long timeStamp)
{
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); //广播地址
	des_addr.sin_port = htons(MASTER_PORT);
	MsgContent_T sendbuf;
	memset(&sendbuf, 0, sizeof(sendbuf));
	sendbuf.type = type;
	sendbuf.len = sizeof(sendbuf) - 1;
	memcpy(sendbuf.timestamp, &timeStamp, sizeof(sendbuf.timestamp));
	sendbuf.s_addr = self_ip();
	dbg_t("native send:");
	MsgContent_T &c = sendbuf;
	print_TimesyncProtocol(c);
	return sendto(recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

ssize_t TimeSync::sendToMaster(RecvMsgType_E type, long timeStamp)
{
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr = master_addr;
	dbg_t("self ip: %s", inet_ntoa(master_addr));
	des_addr.sin_port = htons(MASTER_PORT);
	MsgContent_T sendbuf;
	memset(&sendbuf, 0, sizeof(sendbuf));
	
	sendbuf.type = type;
	sendbuf.len = sizeof(sendbuf) - 1;
	memcpy(sendbuf.timestamp, &timeStamp, sizeof(sendbuf.timestamp));
	sendbuf.s_addr = self_ip();
	return sendto(recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

void TimeSync::send(MsgType type)
{
	dbg_t("mode: %d, type: %d", mode, type);
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
				case WAKEUP:{
					//被唤醒，把事件发送给MASTER
					if(master_addr.s_addr == 0){
						return;
					}
#if 0
					struct sockaddr_in des_addr;
					bzero(&des_addr, sizeof(des_addr));
					des_addr.sin_family = AF_INET;
					des_addr.sin_addr = master_addr;
					dbg_t("self ip: %s", inet_ntoa(master_addr));
					des_addr.sin_port = htons(MASTER_PORT);
					MsgContent_T sendbuf;
					memset(&sendbuf, 0, sizeof(sendbuf));
	
					sendbuf.type = WAKEUP;
					sendbuf.len = sizeof(sendbuf) - 1;
					long cur_time = get_current_timestamp();
					long master_time = time_map_slave_to_master(cur_time);
					dbg_t("cur time :%ld, master time: %ld, c-m = %ld", cur_time, master_time, cur_time-master_time);
					memcpy(sendbuf.timestamp, &master_time, sizeof(sendbuf.timestamp));
					sendbuf.s_addr = self_ip();
					sendto(recv_sockfd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else
					long cur_time = get_current_timestamp();
					long master_time = time_map_slave_to_master(cur_time);
					sendToMaster(WAKEUP, master_time);
#endif
					break;
				}
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
	print_TimesyncProtocol(content);
	switch(mode){
		case MASTER:{
			dbg_t("current is MASTER");
			switch(content.type){
				case BOARDCAST:
					//检查接收内容IP是否和本机一致，否则报错
					dbg_t("self ip: %s", inet_ntoa(self_ip()));
					break;
				case RESPONSE:
					//计算content->ip对应设备与本机的延迟，并保存到队列
					//TODO: 保存对应设备的网络延迟数据，探索计算准确网络延迟的算法
					break;
				case NEGOTIATION:
					break;
				case BET:{
					//发送错误广播，告知本机为MASTER模式
#if 0
					struct sockaddr_in des_addr;
					bzero(&des_addr, sizeof(des_addr));
					des_addr.sin_family = AF_INET;
					des_addr.sin_addr = content.s_addr;
					des_addr.sin_port = htons(MASTER_PORT);
	
					//time_t cur_time = time(NULL);
					long cur_time = get_current_timestamp();
					dbg_t("cur time : %ld, size : %d", cur_time, sizeof(long));
					memcpy(content.timestamp, &cur_time, sizeof(content.timestamp));
					content.s_addr = self_ip();
					content.type = BOARDCAST;
					content.len = sizeof(content) - 1;
					sendto(recv_sockfd, &content, sizeof(content), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else
					long cur_time = get_current_timestamp();
					sendBoardcast(BOARDCAST, cur_time);
#endif
					break;
				}
			}
			break;
		}
		case SLAVE:{
			dbg_t("current is SLAVE");
			switch(content.type){
				case BOARDCAST:{
					//更新系统时间，立即回复BOARDCAST_RESPONSE给content->ip
					master_addr = content.s_addr;
#if 0
					struct sockaddr_in des_addr;
					bzero(&des_addr, sizeof(des_addr));
					des_addr.sin_family = AF_INET;
					des_addr.sin_addr = content.s_addr;
					des_addr.sin_port = htons(MASTER_PORT);
					content.type = BOARDCAST_RESPONSE;
					content.s_addr = self_ip();
					sendto(recv_sockfd, &content, sizeof(MsgContent_T), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else
					sendToMaster(BOARDCAST_RESPONSE, *(long *)content.timestamp);
#endif
					mtimer->deleteTask(timer_fd);
					timer_fd = mtimer->addTask(2*ALIVE_TIME, time_todo, this, 0);
					map_list_add(*(long *)content.timestamp);
					status = NORMAL;
					break;
				}
				case BOARDCAST_RESPONSE:
					//不响应
					break;
				case WAKEUP:
					//响应唤醒事件
					inf("WAKEUP EVENT!");
					break;
				case BET:
					//发送BET广播
					bet(content);
					break;
			}
			break;
		}
		default:
			break;
	}
}

TimeMapList_T *TimeSync::map_list_init(int max)
{
	TimeMapList_T *t = (TimeMapList_T *)malloc(sizeof(TimeMapList_T));
	if(t == NULL){
		err("timeMapList malloc failed");
		return NULL;
	}
	t->max = max;
	t->p = 0;
	t->head = (TimeMap_T *)calloc(max, sizeof(TimeMap_T));
	if(t->head == NULL){
		err("TimeMap calloc failed");
		free(t);
		return NULL;
	}
	return t;
}
		
long TimeSync::time_map_slave_to_master(long slave_time)
{
	if(timeMapList == NULL || timeMapList->head == NULL){
		return -1;
	}
	return *(long *)(timeMapList->head[timeMapList->p].masterTimestamp) + slave_time - *(long *)(timeMapList->head[timeMapList->p].slaveTimestamp);
}

int TimeSync::map_list_add(long timestamp)
{
	if(timeMapList == NULL || timeMapList->head == NULL){
		return -1;
	}
	timeMapList->p++;
	if(timeMapList->p == timeMapList->max){
		timeMapList->p = 0;
	}
	memcpy(timeMapList->head[timeMapList->p].masterTimestamp, &timestamp, sizeof(timeMapList->head[timeMapList->p].masterTimestamp));
	long cur_time = get_current_timestamp();
	dbg_t("master time: %ld, cur time : %ld, c-m: %ld",timestamp, cur_time, cur_time-timestamp);
	memcpy(timeMapList->head[timeMapList->p].slaveTimestamp, &cur_time, sizeof(timeMapList->head[timeMapList->p].slaveTimestamp));
	return 0;
}

void TimeSync::map_list_destory(TimeMapList_T **t)
{
	if(t == NULL || *t == NULL){
		return;
	}
	if((*t)->head){
		free((*t)->head);
		(*t)->head = NULL;
	}
	free(*t);
	*t = NULL;
	return ;
}

void TimeSync::print_TimesyncProtocol(MsgContent_T &t)
{
	int len = (int)t.len;
	struct type_to_str{
		MsgType type; char *type_str;
	}type_str[] = {
		{BOARDCAST, (char *)"BOARDCAST"},
		{BOARDCAST_RESPONSE, (char *)"BOARDCAST_RESPONSE"},
		{WAKEUP, (char *)"WAKEUP"},
		{BET, (char *)"BET"}
	};
	char *type = NULL;
	for(int i = 0; i < sizeof(type_str)/sizeof(struct type_to_str); i++){
		if(type_str[i].type == t.type){
			type = type_str[i].type_str;
		}
	}

	struct tm *tblock;
	long tv_usec = *(long *)t.timestamp;
	tv_usec /= 1000000;
	tblock = localtime((time_t *)&tv_usec);

	dbg_t("len: %d, type: %s, ip: %s, time: %s", len, type, inet_ntoa(t.s_addr), asctime(tblock));
}
