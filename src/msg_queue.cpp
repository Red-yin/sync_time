#include "msg_queue.h"
#include <stdio.h>
#include <stdlib.h>
extern "C"{
	#include "slog.h"
};


MsgQueue::MsgQueue(int qs, enum mode): size(qs), head(NULL), tail(NULL), count(0)
{
    if (pthread_mutex_init(&mutex, NULL))
    {
        err_t("failed to init mutex!\n");
    }
    if(pthread_cond_init(&queue_empty, NULL) || pthread_cond_init(&queue_full, NULL)){
        err_t("msg queue cond init failed");
    }
}

MsgQueue::~MsgQueue()
{
    msg_list_destory();
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&queue_empty);
    pthread_cond_destroy(&queue_full);
}

int MsgQueue::put(void *data)
{
    pthread_mutex_lock(&mutex);
    if(is_full()){
        pthread_cond_wait(&queue_full, &mutex);  //等待队列为空
		dbg_t("data get signal");
    }
    int empty = count == 0;
    int ret = msg_list_add(data);
    if(empty == 1 && ret == 0){
        pthread_cond_signal(&queue_empty);
		dbg_t("data put signal");
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

void *MsgQueue::get()
{
    pthread_mutex_lock(&mutex);
    if(is_empty()){
        pthread_cond_wait(&queue_empty, &mutex);  //等待队列为空
		dbg_t("data get signal");
    }
    bool f = is_full();
    void *ret = msg_list_delete();
    if(f){
        pthread_cond_signal(&queue_full);
		dbg_t("data put signal");
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}
        
bool MsgQueue::is_full() const
{
    return count >= size;
}
bool MsgQueue::is_empty() const
{
    return count==0;
}

void MsgQueue::msg_list_destory()
{
    for(Node *p = head; count > 0; p = head, count--){
        head = head->next;
        free(p);
    }
    count = 0;
    head = tail = NULL;
}

int MsgQueue::msg_list_add(void *data)
{
    if(count >= size){
        printf("list is full: %d, size: %d", count, size);
        return -1;
    }
    Node *node = (Node *)calloc(1, sizeof(Node));
    if(node == NULL){
        printf("msg node calloc failed");
        return -1;
    }
    node->data = data;
    if(count == 0){
        head = tail = node;
    }else{
        tail->next = node;
        tail = node;
    }
    count++;
    return 0;
}

void *MsgQueue::msg_list_delete()
{
    if(count == 0){
        printf("list is empty");
        return NULL;
    }
    Node *node = head;
    head = head->next;
    void *ret = node->data;
    free(node);
    count--;
    return ret;
}