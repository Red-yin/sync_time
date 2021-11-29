#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "cthread.h"

#include "avltree.h"

enum taskType{
	TIMER,
	SOCKET
};

typedef struct taskContent{
	struct avl_node hook;
	int fd;
	int (*handle)(struct taskContent *, void *);
	void *param;
	enum taskType type;
}taskContent, *pTaskContent;

typedef int taskHandler(pTaskContent, void *);

class CTimer: public CThread
{
	public:
		CTimer();
		~CTimer();
		int addTask(int ms, taskHandler *handle, void *param, int repeatFlag);
		int addTask(int fd, taskHandler *handle, void *param);
		int deleteTask(int fd);
		int scanTask();
		taskContent *findTask(int fd);
	private:
		int epollfd;
		void run();
		int epollfdInit(int max);
		int epollAddfd(int fd);
		int epollDeletefd(int fd);

		struct avl_tree *taskBase;
		static avl_cmp_func compare;
		static travel_handle taskContentDestoryOnAvlTree;
		static travel_handle printTask;
};
