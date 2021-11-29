#ifndef __MSG_QUEUE__
#define __MSG_QUEUE__
#include "pthread.h"

typedef struct msg_node{
    void *data;
    struct msg_node *next;
}msg_node_t;

typedef struct msg_list{
    msg_node_t *head;
    msg_node_t *tail;
    int limit;
    int count;
}msg_list_t;

class MsgQueue{
    public:
        enum mode{
            NON_BLOCK,
            BLOCK
        };
        MsgQueue(enum mode=NON_BLOCK);
        ~MsgQueue();
        int put(void *data);
        void *get();
        bool is_empty();
    private:
        pthread_mutex_t mutex;
        pthread_cond_t queue_empty;
        msg_list_t *list;
        msg_list_t *msg_list_init(int limit);
        void msg_list_destory(msg_list_t *);
        int msg_list_add(msg_list_t *list, void *data);
        void *msg_list_delete(msg_list_t *list);
        int get_msg_node_number(msg_list_t *list);
};
#endif