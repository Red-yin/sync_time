#include "my_epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

extern "C"{
#include "slog.h"
};

#define EPOLL_LISTEN_CNT        256
Epoll::Epoll():m_fd(-1),list(MyList<FdData>(destory, compare))
{
	m_fd = epoll_create(100);
}

Epoll::~Epoll()
{
	if(m_fd != -1){
		close(m_fd);
	}
}

void Epoll::run()
{
	int fd_cnt = 0;
	struct epoll_event events[EPOLL_LISTEN_CNT];    
	while(1){
		fd_cnt = epoll_wait(m_fd, events, EPOLL_LISTEN_CNT, -1); 
		dbg_t("trigger count: %d", fd_cnt);
		for(int i = 0; i < fd_cnt; i++){
			int sfd = events[i].data.fd;
			dbg_t("recv fd: %d", sfd);
			if(events[i].events & EPOLLIN){
				CompareData cd;
				cd.type = FD;
				cd.data = &sfd;
				FdData *d = list.find((void *)&cd);
				if(d && d->cbk){
					d->cbk(sfd, d->param);
				}
				list.show();
			}
		}
	}
}


bool Epoll::add(int fd, epoll_cbk cbk, void *param, FdInfo *info, param_destory destory)
{
	if(fd < 0){
		war("%d < 0", fd);
		return false;
	}
	FdData *d = (FdData *)calloc(1, sizeof(FdData));
	if(d == NULL){
		err("FdData calloc failed");
		return false;
	}
	d->fd = fd;
	d->param = param;
	d->cbk = cbk;
	if(info)
		d->info = *info;

	epoll_add_fd(fd);
	list.add(d);
	return true;
}
bool Epoll::del(int fd)
{
	CompareData cd;
	cd.type = FD;
	cd.data = &fd;
	FdData *d = list.find((void *)&cd);
	epoll_delete_fd(fd);
	list.del(d);
	return true;
}

int Epoll::find_fd(struct sockaddr_in *addr)
{
	if(addr == NULL){
		return -1;
	}
	CompareData d;
	d.type = SOCKET_ADDR;
	d.data = addr;
	addr = ((struct sockaddr_in*)&d.data);
	FdData *fd_data = list.find((void *)&d);
	return fd_data ? fd_data->fd: -1;
}

int Epoll::epoll_add_fd(int fd)
{
	int ret;
	struct epoll_event event;

	memset(&event, 0, sizeof(event));
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;

	ret = epoll_ctl(m_fd, EPOLL_CTL_ADD, fd, &event);
	return ret;    
}

int Epoll::epoll_delete_fd(int fd)
{
	int ret = epoll_ctl(m_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	return ret;    

}

bool Epoll::compare(void *param, FdData *data)
{
	CompareData *p = (CompareData *)param;
	switch(p->type){
		case FD:
			return data->fd == *(int*)(p->data);
		case SOCKET_ADDR:
			//return data->addr.sin_port == ((struct sockaddr_in *)(p->data))->sin_port && data->addr.sin_addr.s_addr == ((struct sockaddr_in *)(p->data))->sin_addr.s_addr;
			return !memcmp(&data->info.addr, p->data, sizeof(struct sockaddr_in));
	}
	return false;
}

bool Epoll::destory(FdData *data)
{
	if(data){
		if(data->destory)
			data->destory(data->param);
		free(data);
	}
	return true;
}
