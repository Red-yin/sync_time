//#include <pthread.h>
#include "cthread.h"
#include "ctimer.h"
#include "thread_pool.h"
#include "msg_queue.h"
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>

#define CONFIG_DEBUG 1
typedef enum RunMode{
	MASTER,		//主模式
	SLAVE,		//从模式
	NEGOTIATION	//协商模式，当主模式冲突时进入协商模式解决冲突 
}RunMode_E;

typedef enum MsgType{
	BOARDCAST, // 主模式：发送广播消息；从模式：接收广播消息
	RESPONSE,//主模式：接收广播消息对应的单播回复；从模式：发送对广播消息的回复
	BET  //发送方：发起竞选；接收方：响应竞选，赢了再次发起竞选，输了不响应且进入等待广播状态
}RecvMsgType_E;

//协议数据格式
typedef struct MsgContent{
	char len;
	char type;
	in_addr s_addr;
	char timestamp[8];
	char index[8];
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

typedef struct TimeDelay{
	long master_timestamp;
	long time_delay;
	struct TimeDelay *next;
}TimeDelay_T;

typedef struct ItemTimeDelay{
	in_addr s_addr;		//设备IP地址
	TimeDelay_T *head;	//最老的数据
	TimeDelay_T *tail;	//最新的数据
	int count;			//存储的数据个数
	struct ItemTimeDelay *next;
}ItemTimeDelay_T;

typedef struct SlaveDeviceList{
	ItemTimeDelay_T *head;
	int count;
}SlaveDeviceList_T;

class TimeSync : public CThread
{
	private:
		RunMode_E mode;
		TimeSyncStatus_E status;
		//pthread_t pt;
		int multicast_fd;
		int unicast_fd;
		CTimer *mtimer;
		int timer_fd; 
		//MASTER时间和本机时间映射
		TimeMapList_T *timeMapList;
		TimeMapList_T *map_list_init(int max);
		int map_list_add(long timestamp);
		void map_list_destory(TimeMapList_T **t);
		long time_map_slave_to_master(long slave_time);

		//所有SLAVE设备的网络延迟数据
		SlaveDeviceList_T *device_td_list;

		int bet(MsgContent_T &content);

		WakeupManage_T *wakeup;
		in_addr master_addr;

		ssize_t sendToMaster(RecvMsgType_E type, long timeStamp);
		ssize_t sendBoardcast(RecvMsgType_E type, long timeStamp);
		void handle(RunMode_E mode, MsgContent &content);
		void run();
		in_addr self_ip();//获取本机IP
		long get_current_timestamp();
#ifdef CONFIG_DEBUG
		void print_TimesyncProtocol(MsgContent_T &t);
#endif

		ThreadPool *tp;
		MsgQueue *mq;

		static int time_todo(pTaskContent task, void *param);
		static int wakeup_response(pTaskContent task, void *param);
		static void *recv_multicast(void *param);
		static void *recv_unicast(void *param);
		static void *msg_handle(void *param);
	public:
		TimeSync();
		~TimeSync();
		void send(MsgType type);
};
