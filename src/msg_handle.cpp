#include "msg_handle.h"

MsgHandle::MsgHandle(MsgQueue *q, void (*cbk)(void *param, void *data), void *param):
	queue(q), handle(cbk), param(param)
{
	t = std::move(std::thread(wait, this));
}
		
MsgHandle::~MsgHandle()
{
	t.join();
}

void MsgHandle::wait(void *param)
{
	MsgHandle *mh = (MsgHandle *)param;
	while(1){
		void *d = mh->queue->get();
		if(mh->handle)
			mh->handle(mh->param, d);
	}
}
