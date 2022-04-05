#include "sync_time.h"
#include <unistd.h>

int main(void)
{
	SyncTime *st = new SyncTime();
#if 1
	st->run();
#else
	t->send("192.168.134.131", 10000, (const void *)"hello", 5);
	sleep(1);
	t->send("192.168.134.131", 10000, (const void *)"12345", 5);
	sleep(1);
	t->over("192.168.134.131", 10000);
#endif
    return 0;
}
