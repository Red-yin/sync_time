
//#include <pthread.h>
#include "cthread.h"
typedef struct TimeSyncProtocol{
	char len;
	char type;
	char ip[4];
	char timestamp[8];
}TimeSyncProtocol_T;

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
	char ip[16];
	long timestamp;
	RecvMsgType_E type;
}MsgContent_T;

class TimeSync : public CThread
{
	private:
		RunMode_E mode;
		//pthread_t pt;
		int recv_sockfd;
		int send_sockfd;
		void* recv(void *);
		void send(MsgType type, RunMode_E mode);
		void handle(RunMode_E mode, MsgContent &content);
		void run();
	public:
		TimeSync();
		~TimeSync();
};
