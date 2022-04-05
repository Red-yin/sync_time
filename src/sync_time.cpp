#include "sync_time.h"

#define PERIOD 1000
SyncTime::SyncTime():
	status(SLAVE), timer_fd(-1)
{
	port = 10121;
	ep = new Epoll();
	q = new MsgQueue(1024, MsgQueue::BLOCK);
	mh = new MsgHandle(q, msg_handle, this);
	timer = new Timer(ep);
	t_io = new TcpIO(port, ep, q);
	u_io = new UdpIO("224.0.1.1", port, ep, q);
	//i_io = new IcmpIO(ep);

	switch_to_slave();

	srand((unsigned)get_current_timestamp());
	index = rand();
}

SyncTime::~SyncTime()
{
}

void SyncTime::run()
{
	ep->run();
}

void SyncTime::switch_to_slave()
{
	status = SLAVE;
	timer->del(timer_fd);
	timer_fd = timer->add(PERIOD *2, time_to_do, (void *)this, 1);
}

void SyncTime::switch_to_master()
{
	status = MASTER;
	timer_fd = timer->add(PERIOD, time_to_do, (void *)this, -1);
}

void SyncTime::time_to_do(void *param)
{
	SyncTime *st = (SyncTime *)param;
	switch(st->status){
		case MASTER:
			st->send_master_data();
			break;
		case SLAVE:
			st->switch_to_master();
			break;
	}
}

void SyncTime::msg_handle(void *param, void *data)
{
	SyncTime *st = (SyncTime *)param;
	MsgContent_T *p = (MsgContent_T *)data;
	st->handle(p);
}

void SyncTime::handle(MsgContent_T *data)
{
	switch(status){
		//当前状态
		case MASTER:{
			switch(data->type){
				case TIME_SYNC:
					//发起竞选
					send_bet(data);
					break;
				case RESPONSE:{
					long time_now = get_current_timestamp();
					long gap = time_now = time_now - *(long *)data->timestamp;
					dbg_t("time gap: %ldus", gap);
					print_TimesyncProtocol(data);
					break;
				}
				case BET:
					//竞选结果回复
					response_bet(data);
					break;
			}
			break;
		}
		case SLAVE:{
			switch(data->type){
				case TIME_SYNC:
					response_time_sync(data);
					refresh_slave_timer();
					sync_time(data);
					break;
				case RESPONSE:
					//do nothing
					break;
				case BET:
					//do nothing
					break;
			}
			break;
		}
	}
}

void SyncTime::response_time_sync(MsgContent_T *c)
{
	char des_ip[32] = {0};
	inet_ntop(AF_INET, &c->s_addr, des_ip, sizeof(des_ip));
	dbg_t("response to ip: %s", des_ip);

	MsgContent_T data;
	memset(&data, 0, sizeof(data));
	data.type = RESPONSE;
	data.s_addr = t_io->self_ip();
	data.len = sizeof(MsgContent_T) - 1;

	memcpy(data.index, c->index, sizeof(data.index));
	memcpy(data.timestamp, c->timestamp, sizeof(data.timestamp));
	if(0 > u_io->send(des_ip, port, (void *)&data, sizeof(MsgContent_T)))
		err("%s", strerror(errno));
}

void SyncTime::send_bet(MsgContent_T *c)
{
	char des_ip[32] = {0};
	inet_ntop(AF_INET, &c->s_addr, des_ip, sizeof(des_ip));
	dbg_t("send to ip: %s", des_ip);

	MsgContent_T data;
	memset(&data, 0, sizeof(data));
	data.type = BET;
	data.s_addr = t_io->self_ip();
	data.len = sizeof(MsgContent_T) - 1;

	srand(*((unsigned int *)c->timestamp));
	int num = rand();
	memcpy(data.index, &num, sizeof(data.index));
	t_io->send(des_ip, port, (void *)&data, sizeof(MsgContent_T));
}

void SyncTime::response_bet(MsgContent_T *c)
{
	unsigned client_num = *((unsigned*)c->index);
	int seed = (int)get_current_timestamp();
	srand(seed);
	unsigned local_num = rand();
	dbg_t("local num: %u, bet num: %u", local_num, client_num);
	if(local_num > client_num){
		inf_t("bet: win");
		char des_ip[32] = {0};
		inet_ntop(AF_INET, &c->s_addr, des_ip, sizeof(des_ip));
		dbg_t("send to ip: %s", des_ip);

		MsgContent_T data;
		memset(&data, 0, sizeof(data));
		data.type = BET;
		data.s_addr = t_io->self_ip();
		data.len = sizeof(MsgContent_T) - 1;
		*((long long*)data.index) |= 0xffffffffffffffff;
		t_io->send(des_ip, port, (void *)&data, sizeof(MsgContent_T));
	}else{
		inf_t("bet: lost");
		switch_to_slave();
	}
}

void SyncTime::refresh_slave_timer()
{
	dbg_t("current status: %d", status);
	timer->del(timer_fd);
	timer_fd = timer->add(PERIOD *2, time_to_do, (void *)this, 1);
}

void SyncTime::send_master_data()
{
	MsgContent_T data;
	memset(&data, 0, sizeof(data));
	data.type = TIME_SYNC;
	data.s_addr = t_io->self_ip();
	data.len = sizeof(MsgContent_T) - 1;
	long timestamp = get_current_timestamp();
	memcpy(data.index, &index, sizeof(index));
	memcpy(data.timestamp, &timestamp, sizeof(data.timestamp));
	index++;
	if(0 > u_io->send(&data, sizeof(MsgContent_T))){
		err("%s", strerror(errno));
	}
}

void SyncTime::sync_time(MsgContent_T *c)
{
	print_TimesyncProtocol(c);
}

long SyncTime::get_current_timestamp()
{
	struct timeval tv;
    gettimeofday(&tv, NULL);
	long cur_time = tv.tv_sec * 1000000 + tv.tv_usec;
	return cur_time;
}

void SyncTime::print_TimesyncProtocol(MsgContent_T *t)
{
	int len = (int)t->len;
	struct type_to_str{
		MsgType type; char *type_str;
	}type_str[] = {
		{TIME_SYNC, (char *)"TIME_SYNC"},
		{RESPONSE, (char *)"RESPONSE"},
		{BET, (char *)"BET"}
	};
	char *type = NULL;
	for(int i = 0; i < sizeof(type_str)/sizeof(struct type_to_str); i++){
		if(type_str[i].type == t->type){
			type = type_str[i].type_str;
		}
	}

	struct tm *tblock;
	long tv_usec = *(long *)t->timestamp;
	dbg_t("index : %ld", *(long *)t->index);
	dbg_t("sync time: %ld ns", tv_usec);
	tv_usec /= 1000000;
	tblock = localtime((time_t *)&tv_usec);

	dbg_t("len: %d, type: %s, ip: %s, time: %s", len, type, inet_ntoa(t->s_addr), asctime(tblock));
}
