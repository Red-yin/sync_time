#include "msg_queue.h"
#include <stdio.h>
#include <stdlib.h>

#define MSG_LIST_LIMIT 1024

MsgQueue::MsgQueue(enum mode)
{
    list = msg_list_init(MSG_LIST_LIMIT);
    if (pthread_mutex_init(&mutex, NULL))
    {
        printf("failed to init mutex!\n");
    }
}

MsgQueue::~MsgQueue()
{
    msg_list_destory(list);
    pthread_mutex_destroy(&mutex);
}

int MsgQueue::put(void *data)
{
    pthread_mutex_lock(&mutex);
    int empty = list->count == 0;
    int ret = msg_list_add(list, data);
    if(empty == 1 && ret == 0){
        pthread_cond_signal(&queue_empty);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

void *MsgQueue::get()
{
    pthread_mutex_lock(&mutex);
    if(list->count == 0){
        pthread_cond_wait(&queue_empty, &mutex);  //等待队列为空
        pthread_mutex_lock(&mutex);
    }
    void *ret = msg_list_delete(list);
    pthread_mutex_unlock(&mutex);
    return ret;
}
        
bool MsgQueue::is_empty()
{
    pthread_mutex_lock(&mutex);
    int ret = get_msg_node_number(list);
    pthread_mutex_unlock(&mutex);
    return ret==0;
}

msg_list_t *MsgQueue::msg_list_init(int limit)
{
    msg_list_t *l = (msg_list_t *)calloc(1, sizeof(msg_list_t));
    if(l == NULL){
        printf("calloc failed");
        return NULL;
    }
    l->head = NULL;
    l->tail = NULL;
    l->limit = limit;
    l->count = 0;
    return l;
}

void MsgQueue::msg_list_destory(msg_list_t *l)
{
    if(l == NULL){
        return ;
    }
    for(msg_node_t *p = l->head; l->count > 0; p = l->head, l->count--){
        l->head = l->head->next;
        free(p);
    }
    free(l);
}

int MsgQueue::msg_list_add(msg_list_t *list, void *data)
{
    if(list == NULL){
        printf("error: list is NULL");
        return -1;
    }
    if(list->count >= list->limit){
        printf("list is full: %d, limit: %d", list->count, list->limit);
        return -1;
    }
    msg_node_t *node = (msg_node_t*)calloc(1, sizeof(msg_list_t));
    if(node == NULL){
        printf("msg node calloc failed");
        return -1;
    }
    node->data = data;
    if(list->count == 0){
        list->head = list->tail = node;
    }else{
        list->tail->next = node;
        list->tail = node;
    }
    list->count++;
    return 0;
}

void *MsgQueue::msg_list_delete(msg_list_t *list)
{
    if(list == NULL){
        printf("list is NULL");
        return NULL;
    }
    if(list->count == 0){
        printf("list is empty");
        return NULL;
    }
    msg_node_t *node = list->head;
    list->head = list->head->next;
    void *ret = node->data;
    free(node);
    return ret;
}
int MsgQueue::get_msg_node_number(msg_list_t *list)
{
    return list->count;
}