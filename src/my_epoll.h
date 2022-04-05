#ifndef MY_EPOLL_H__
#define MY_EPOLL_H__

#include "my_list.h"
#include <arpa/inet.h>

class Epoll
{
	public:
		Epoll();
		~Epoll();
		typedef void (*epoll_cbk)(int fd, void *param);
		enum FdType{
			SERVER,
			CLIENT
		};
		typedef struct FdInfo{
			enum FdType type;
			long born_time;
			struct sockaddr_in addr; 
		}FdInfo;
		typedef void (*param_destory)(void *);
		bool add(int fd, epoll_cbk cbk, void *param, FdInfo *info=NULL, param_destory destory = NULL);
		bool del(int fd);
		int find_fd(struct sockaddr_in *addr);
		void run();
	private:
		int m_fd;
		enum CompareType{
			FD,
			SOCKET_ADDR
		};
		typedef struct CompareData{
			void *data;
			enum CompareType type;
		}CompareData;
		typedef struct FdData{
			int fd;
			void *param;
			epoll_cbk cbk;
			param_destory destory;
			FdInfo info;
		}FdData;
		MyList<FdData> list;

		int epoll_add_fd(int fd);
		int epoll_delete_fd(int fd);

		static bool compare(void *param, FdData *data);
		static bool destory(FdData *data);
};
#endif
