#ifndef __MSGIO_H__
#define __MSGIO_H__

#include <thread>
#include "thread_pool.h"
#include "msg_queue.h"
using namespace std;

class MsgIO{
    private:
        enum MsgIOMode{
            MASTER,
            CLIENT
        };
        ThreadPool *tp;
        MsgQueue *mq;

        void init_master();
        void init_client();
        void destory();
        thread udp_input;
        thread tcp_input;
        thread udp_output;
        thread tcp_output;
        int init_multicast(int port);
        int init_unicast(int port);

		static void send_msg(void *arg);
        static void *tcp_recv(void *arg);
        static void recv_multicast_msg(void *arg);
        static void recv_unicast_msg(void *arg);
        static void send_multicast_msg(void *arg);
        static void send_upd_msg(void *arg);
        static void send_tcp_msg(void *arg);
    public:
        MsgIO(MsgIOMode mode=MASTER);
        ~MsgIO();
};
#endif
