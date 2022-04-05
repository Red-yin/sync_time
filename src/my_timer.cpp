#include "my_timer.h"
#include <sys/timerfd.h>
#include <unistd.h>

Timer::Timer(Epoll *ep):ep(ep)
{
}

Timer::~Timer()
{
}

int Timer::add(unsigned ms, timer_cbk cbk, void *param, int times, param_destory destory)
{
	if(cbk == NULL){
		return -1;
	}

	int fd = timerfd_create(CLOCK_MONOTONIC, 0);
	int ret;
	struct itimerspec new_value;
	memset(&new_value, 0, sizeof(new_value));
	new_value.it_value.tv_sec = ms/1000;
	new_value.it_value.tv_nsec = (ms%1000)*1000*1000;
	if(times != 1){
		//struct itimerspec结构体中it_interval参数全部为0时，表示定时器是非周期型的；
		new_value.it_interval.tv_sec = new_value.it_value.tv_sec;
		new_value.it_interval.tv_nsec = new_value.it_value.tv_nsec;
	}

	ret = timerfd_settime(fd, 0, &new_value, NULL);
	if (ret < 0) {
		err_t("timerfd_settime error, Error:[%d:%s]", errno, strerror(errno));
		close(fd);
		return ret;
	}
	TimerData *p = (TimerData *)calloc(1, sizeof(TimerData));
	if(p == NULL){
		err_t("TimerData calloc failed");
		close(fd);
		return -1;
	}
	p->cbk = cbk;
	p->param = param;
	p->destory = destory;
	p->times = times;
	p->fd = fd;
	p->ep = ep;
	ep->add(fd, timer_handle, (void *)p, NULL, destory);

	return fd;
}

bool Timer::del(int fd)
{
	return ep->del(fd);
}

void Timer::timer_handle(int fd, void *param)
{
	uint64_t exp = 0;
	int n = read(fd, &exp, sizeof(uint64_t)); 
	dbg_t("timer fd %d read %d bytes, %lu", fd, n, exp);

	if(param == NULL){
		return;
	}
	TimerData *p = (TimerData *)param;
	if(p->cbk)
		p->cbk(p->param);
	if(p->times > 0){
		p->times--;
	}
	if(p->times == 0){
		if(p->ep)
			p->ep->del(fd);
	}
}

void Timer::destory(void *data)
{
	TimerData *d = (TimerData *)data;
	if(data){
		if(d->destory)
			d->destory(d->param);
		free(data);
	}
}
