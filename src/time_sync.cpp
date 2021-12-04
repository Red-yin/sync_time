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
#include <unistd.h>

extern "C"{
	#include "slog.h"
};
//#include <thread>

#define MASTER_PORT 7655
#define ALIVE_TIME 1000

TimeSync::TimeSync()
{
	mode = SLAVE;
	dbg_t("mode: SLAVE");
	status = NORMAL;
	mtimer = new CTimer();
	//timer_fd = mtimer->addTask(3*ALIVE_TIME, time_todo, this, 0);
	memset(&master_addr, 0, sizeof(master_addr));

	timeMapList = map_list_init(10);

	struct sockaddr_in des_addr;

	multicast_fd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	//des_addr.sin_addr.s_addr = inet_addr("255.255.255.255"); //广播地址
	des_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	des_addr.sin_port = htons(MASTER_PORT);
	 /* 设置通讯方式广播，即本程序发送的一个消息，网络上所有的主机均可以收到 */
    setsockopt(multicast_fd,SOL_SOCKET,SO_BROADCAST,&multicast_fd,sizeof(multicast_fd));
	if (bind(multicast_fd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0){
		err("socket bind failed\n");
	}

	unicast_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(unicast_fd, (struct sockaddr *)&des_addr, sizeof(des_addr)) < 0){
		err("socket bind failed\n");
	}
	if(listen(unicast_fd, 256) == -1) {
		err("Listen error:%s", strerror(errno));
		exit(1);
	}
	tp = new ThreadPool(5, 10);
	tp->add_job(recv_multicast, this);
	//tp->add_job(recv_unicast, this);
	tp->add_job(msg_handle, this);

	mq = new MsgQueue();
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
	MsgContent_T *content = NULL;
	char recvbuf[32] = {0};
	struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
	socklen_t client_len;
	int n = 0;
	while(1){
		dbg_t("current mode: %d", mode);
		switch(mode){
			case MASTER:{
				//一个周期发送一个组播消息
				sleep(1);
				long cur_time = get_current_timestamp();
				sendMulticast(BOARDCAST, cur_time);
				break;
			}
			case SLAVE:
				//几个周期检查一次组播消息是否正常
				sleep(3);
				if(is_master_exist() == false){
					mode_switch(MASTER);
				}else{
					set_master_exist(false);
				}
				break;
			case NEGOTIATION:
				sleep(1);
				long cur_time = get_current_timestamp();
				sendBoardcast(BOARDCAST, cur_time);
				break;
		}
	}
}

bool TimeSync::is_master_exist()
{
	return master_exist;
}

void TimeSync::mode_switch(RunMode_E m)
{
	mode = m;
}

void TimeSync::set_master_exist(bool b)
{
	master_exist = b;
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
	ioctl(multicast_fd, SIOCGIFCONF, (char *)&ifc);
	int intr = ifc.ifc_len / sizeof(struct ifreq);
	while (intr-- > 0 && ioctl(multicast_fd, SIOCGIFADDR, (char *)&buf[intr]));

	return ((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr;
}

void *TimeSync::recv_multicast(void *param)
{
	TimeSync *as = (TimeSync *)param;
	while(1){
		MsgContent_T *content = (MsgContent_T *)calloc(1, sizeof(MsgContent_T));
		socklen_t client_len;
		struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
		bzero(&client_addr, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		client_addr.sin_port = htons(MASTER_PORT);
	
		int n = 0;
		client_len = sizeof(client_addr);
		n = recvfrom(as->multicast_fd, (char *)content, sizeof(MsgContent_T), 0, (struct sockaddr *)&client_addr, &client_len);
		dbg_t("recvfd: %d %d",as->multicast_fd, n);
		if(n > 0){
			MsgFrom_T *mf = (MsgFrom_T *)calloc(1, sizeof(MsgFrom_T));
			if(mf == NULL){
				printf("error: MsgFrom_T calloc failed");
				continue;
			}
			mf->fd = as->multicast_fd;
			mf->data = content;
			as->mq->put((void *)mf);
		}
	}
	return 0;
}

void *TimeSync::recv_unicast(void *param)
{
	TimeSync *as = (TimeSync *)param;
	while(1){
		MsgContent_T *content = (MsgContent_T *)calloc(1, sizeof(MsgContent_T));
		socklen_t client_len;
		struct sockaddr_in client_addr;  //client_addr用于记录发送方的地址信息
		bzero(&client_addr, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		client_addr.sin_port = htons(MASTER_PORT);
		int n = 0;
		client_len = sizeof(client_addr);
		n = recvfrom(as->unicast_fd, (char *)content, sizeof(MsgContent_T), 0, (struct sockaddr *)&client_addr, &client_len);
		dbg_t("recvfd: %d %d",as->multicast_fd, n);
		if(n > 0){
			MsgFrom_T *mf = (MsgFrom_T *)calloc(1, sizeof(MsgFrom_T));
			if(mf == NULL){
				printf("error: MsgFrom_T calloc failed");
				continue;
			}
			mf->fd = as->multicast_fd;
			mf->data = content;
			as->mq->put((void *)mf);
		}
	}
	return 0;
}

void *TimeSync::msg_handle(void *param)
{
	TimeSync *as = (TimeSync *)param;
	while(1){
		MsgFrom_T * content = (MsgFrom_T*)as->mq->get();
		dbg_t("content get");
		as->handle(as->mode, content);
	}
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
			sendto(t->multicast_fd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
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
					sendto(t->multicast_fd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
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
					sendto(t->multicast_fd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
#else
					long cur_time = t->get_current_timestamp();
					t->sendBoardcast(BOARDCAST, cur_time);
#endif
					break;
				}
			}
			break;
		}
	}
	return 0;
}

int TimeSync::bet(MsgContent_T &content)
{
	return 0;
}

ssize_t TimeSync::sendMulticast(MsgType_E type, long timeStamp)
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
	return sendto(multicast_fd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

ssize_t TimeSync::sendUnicastUdp(in_addr *addr, MsgType_E type, long timeStamp)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr = *addr;
	des_addr.sin_port = htons(MASTER_PORT);

	MsgContent_T content;
	content.len = sizeof(MsgContent_T) - 1;
	content.type = type;
	content.s_addr = self_ip();
	*(long *)content.timestamp = timeStamp;

	MsgContent_T &c = content;
	print_TimesyncProtocol(c);

	return sendto(fd, &content, sizeof(MsgContent_T), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}


ssize_t TimeSync::sendUnicastTcp(in_addr *addr, MsgType_E type, long timeStamp)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in des_addr;
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr = *addr;
	des_addr.sin_port = htons(MASTER_PORT);

	MsgContent_T content;
	content.len = sizeof(MsgContent_T) - 1;
	content.type = type;
	content.s_addr = self_ip();

	MsgContent_T &c = content;
	print_TimesyncProtocol(c);

	return sendto(fd, &content, sizeof(MsgContent_T), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

ssize_t TimeSync::sendBoardcast(MsgType_E type, long timeStamp)
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
	return sendto(multicast_fd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

ssize_t TimeSync::sendToMaster(MsgType_E type, long timeStamp)
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
	return sendto(multicast_fd, &sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&des_addr, sizeof(des_addr));
}

void TimeSync::handle(RunMode_E mode, MsgFrom_T *mf)
{
	MsgContent &content = *mf->data;
	print_TimesyncProtocol(content);
	switch(mode){
		case MASTER:{
			dbg_t("recv msg, state is MASTER");
			if(mf->fd == multicast_fd){
			switch(content.type){
				case BOARDCAST:{
					//检查接收内容IP是否和本机一致，否则报错
					//发送单播消息给消息源头
					char ip[16] = {0};
					char *p = inet_ntoa(self_ip());
					dbg_t("self ip: %s", p);
					strcpy(ip, p);
					p = inet_ntoa(content.s_addr);
					dbg_t("recv ip: %s", p);
					if(strcmp(ip, p) != 0){
						sendUnicastUdp(&content.s_addr, BET, *(long *)content.timestamp);
					}
					break;
				}
				case RESPONSE:
					//不可能出现的情况
					break;
				case BET:{
					//不可能出现的情况
					break;
				}
			}
			}else if(mf->fd == unicast_fd){
			switch(content.type){
				case BOARDCAST:
					//不可能出现的情况
					break;
				case RESPONSE:
					//计算content->ip对应设备与本机的延迟，并保存到队列
					//TODO: 保存对应设备的网络延迟数据，探索计算准确网络延迟的算法
					break;
				case BET:{
					//开始竞选流程
					break;
				}
			}
			}
			break;
		}
		case SLAVE:{
			dbg_t("recv msg, state is SLAVE");
			if(mf->fd == multicast_fd){
			switch(content.type){
				case BOARDCAST:{
					//更新系统时间，立即回复RESPONSE给content->ip
					set_master_exist(true);
					master_addr = content.s_addr;
					sendUnicastUdp(&content.s_addr, RESPONSE, *(long *)content.timestamp);
					map_list_add(*(long *)content.timestamp);
					break;
				}
				case RESPONSE:
					//不可能出现的情况
					break;
				case BET:
					//不可能出现的情况
					break;
			}
			}else if(mf->fd == unicast_fd){
			switch(content.type){
				case BOARDCAST:{
					//不可能出现的情况
					break;
				}
				case RESPONSE:
					//不可能出现的情况
					break;
				case BET:
					//不可能出现的情况
					break;
			}
			}
			break;
		}
		case NEGOTIATION:{
			if(mf->fd == multicast_fd){
			switch(content.type){
				case BOARDCAST:{
					//回复竞选单播消息
					break;
				}
				case RESPONSE:
					//不可能出现的情况
					break;
				case BET:
					//不可能出现的情况
					break;
			}
			}else if(mf->fd == unicast_fd){
			switch(content.type){
				case BOARDCAST:{
					//不可能出现的情况
					break;
				}
				case RESPONSE:
					//记录从机的数据
					break;
				case BET:
					//开始竞选流程
					break;
			}
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
		{RESPONSE, (char *)"RESPONSE"},
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
