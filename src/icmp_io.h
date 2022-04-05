#ifndef ICMP_IO_H__
#define ICMP_IO_H__

#include "my_epoll.h"
class IcmpIO
{
	private:
		int fd;
		Epoll *epoll;
		void icmp_printf(void *data, int len);
	public:
		IcmpIO(Epoll *ep);
		~IcmpIO();
		static void recv(int fd, void *param);
};

#endif
