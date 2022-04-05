#ifndef MSG_HANDLE_H__
#define MSG_HANDLE_H__

#include "msg_queue.h"
#include <thread>
class MsgHandle
{
	public:
		MsgHandle(MsgQueue *q, void (*cbk)(void *param, void *data), void *param);
		~MsgHandle();
	private:
		std::thread t;
		static void wait(void *param);
		MsgQueue *queue; 
		void (*handle)(void *param, void *data);
		void *param;
};

#endif
