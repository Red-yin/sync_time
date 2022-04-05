#ifndef MY_TIMER_H__
#define MY_TIMER_H__

#include "my_epoll.h"

class Timer
{
	public:
		Timer(Epoll *ep);
		~Timer();
		typedef void (*timer_cbk)(void *param);
		typedef void (*param_destory)(void *param);
		int add(unsigned ms, timer_cbk cbk, void *param, int times, param_destory destory = NULL);
		bool del(int fd);
	private:
		Epoll *ep;
		static void destory(void *param);
		static void timer_handle(int fd, void *param);
		typedef struct TimerData{
			timer_cbk cbk;
			void *param; 
			param_destory destory;
			int times;
			int fd;
			Epoll *ep;
		}TimerData;
};

#endif
