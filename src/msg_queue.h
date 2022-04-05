#ifndef MSG_QUEUE_H_
#define MSG_QUEUE_H_
#include "pthread.h"

class MsgQueue{
    private:
        struct Node
        {
            void *data;
            struct Node *next;
        };
        enum {Q_SIZE = 1024};
        
        Node *head;
        Node *tail;
        int count;
        const int size;

        pthread_mutex_t mutex;
        pthread_cond_t queue_empty;
        pthread_cond_t queue_full;
        bool is_empty() const;
        bool is_full() const;
        void msg_list_destory();
        int msg_list_add(void *data);
        void *msg_list_delete();
    public:
        enum mode{
            NON_BLOCK,
            BLOCK
        };
        MsgQueue(int qs=Q_SIZE, enum mode=NON_BLOCK);
        ~MsgQueue();
        int put(void *data);
        void *get();
};
#endif