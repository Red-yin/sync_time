#ifndef SYNC_TIME_H__
#define SYNC_TIME_H__

#include "tcp_io.h"
#include "udp_io.h"
#include "icmp_io.h"
#include "my_timer.h"
#include "msg_handle.h"

class SyncTime
{
	private:
		enum Status{
			MASTER,
			SLAVE
		};
		enum Status status;
		UdpIO *u_io;
		TcpIO *t_io; 
		IcmpIO *i_io; 
		Timer *timer;
		Epoll *ep;
		MsgQueue *q;
		MsgHandle *mh;
		int timer_fd;
		long unsigned index;

		typedef enum MsgType:char{
			TIME_SYNC, // 主模式：发送广播消息；从模式：接收广播消息
			RESPONSE,//主模式：接收广播消息对应的单播回复；从模式：发送对广播消息的回复
			BET  //发送方：发起竞选；接收方：响应竞选，赢了再次发起竞选，输了不响应且进入等待广播状态
		}MsgType_E;

		//协议数据格式
		typedef struct MsgContent{
			char len;
			MsgType_E type;
			in_addr s_addr;
			char timestamp[8];
			char index[8];
		}MsgContent_T;
		int port;
		void print_TimesyncProtocol(MsgContent_T *t);
		void send_bet(MsgContent_T *c);
		void response_bet(MsgContent_T *c);
		void response_time_sync(MsgContent_T *c);
		void sync_time(MsgContent_T *c);

		void refresh_slave_timer();
		void send_master_data();
		void switch_to_slave();
		void switch_to_master();
		static void time_to_do(void *param);
	public:
		SyncTime();
		~SyncTime();
		long get_current_timestamp();
		void handle(MsgContent_T *data);
		static void msg_handle(void *param, void *data);
		void run();
};

#endif
