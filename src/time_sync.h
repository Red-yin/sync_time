//#include <pthread.h>
#include "cthread.h"
#include "ctimer.h"
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>

#define CONFIG_DEBUG 1
typedef enum RunMode{
	MASTER,
	SLAVE
}RunMode_E;

typedef enum MsgType{
	BOARDCAST, // 主模式：发送广播消息；从模式：接收广播消息
	BOARDCAST_RESPONSE,//主模式：接收广播消息对应的单播回复；从模式：发送对广播消息的回复
	WAKEUP,//主模式：接收方，从模式设备被唤醒的事件，发送方，发送目的设备响应唤醒事件；从模式，发送方，唤醒被触发，接收方，唤醒事件可以响应；
	BET  //发送方：发起竞选；接收方：响应竞选，赢了再次发起竞选，输了不响应且进入等待广播状态
}RecvMsgType_E;

typedef struct MsgContent{
	char len;
	char type;
	in_addr s_addr;
	char timestamp[8];
}MsgContent_T;

typedef struct TimeMap{
	char masterTimestamp[8];
	char slaveTimestamp[8];
}TimeMap_T;

typedef struct TimeMapList{
	int max;
	int p;
	TimeMap_T *head;
}TimeMapList_T;

typedef enum TimeSyncStatus{
	NORMAL,
	WAIT_FOR_BET_RESULT
}TimeSyncStatus_E;

typedef struct TimeGapManage{
	long timestamp;
}TimeGapManage_T;

typedef struct WakeupManage{
	int event_flag;
	int timer_fd;
	in_addr s_addr;
	char timestamp[8];
}WakeupManage_T;

class TimeSync : public CThread
{
	private:
		RunMode_E mode;
		TimeSyncStatus_E status;
		//pthread_t pt;
		int recv_sockfd;
		int send_sockfd;
		CTimer *mtimer;
		int timer_fd; 
		//MASTER时间和本机时间映射
		TimeMapList_T *timeMapList;
		TimeMapList_T *map_list_init(int max);
		int map_list_add(long timestamp);
		void map_list_destory(TimeMapList_T **t);
		long time_map_slave_to_master(long slave_time);

		WakeupManage_T *wakeup;

		void* recv(void *);
		void send(MsgType type, RunMode_E mode);
		void handle(RunMode_E mode, MsgContent &content);
		void run();
		in_addr self_ip();//获取本机IP
		long get_current_timestamp();
#ifdef CONFIG_DEBUG
		void print_TimesyncProtocol(MsgContent_T &t);
#endif

		static int time_todo(pTaskContent task, void *param);
		static int wakeup_response(pTaskContent task, void *param);
	public:
		TimeSync();
		~TimeSync();
};
