#ifndef MY_LIST_H__
#define MY_LIST_H__

#include <stdio.h>
#include <stdlib.h>
extern "C"{
#include "slog.h"
};

template <typename Type>
class MyList
{
	public:
		typedef bool (*NodeDestory)(Type *data);
		typedef bool (*DataCompare)(void *param, Type *data);
		MyList();
		MyList(NodeDestory destory, DataCompare compare);
		~MyList();
		bool add(Type *data);
		bool del(Type *data);
		Type *find(void *param);
		void show();
		void register_destory(NodeDestory destory);
		void register_compare(DataCompare compare);
	private:
		typedef struct Data{
			Type *node;
			struct Data *next;
		}Data;
		struct ListHead{
			Data *head;
			NodeDestory destory;
			DataCompare compare;
#ifdef DEBUG
			unsigned count;
#endif
		}data_manager;
		void destory_data_list();
		void destory_data_node(Data *node);
};

template <typename Type>
MyList<Type>::MyList()
{
#ifdef DEBUG
	data_manager.count = 0;
#endif
}


template <typename Type>
MyList<Type>::MyList(NodeDestory destory, DataCompare compare)
{
	register_destory(destory);
	register_compare(compare);
}

template <typename Type>
MyList<Type>::~MyList()
{
	destory_data_list();
}

template <typename Type>
void MyList<Type>::destory_data_node(Data *node)
{
	if(node == NULL){
		return;
	}
	if(data_manager.destory != NULL){
		data_manager.destory(node->node);
	}
	free(node);
}

template <typename Type>
void MyList<Type>::destory_data_list()
{
	while(data_manager.head != NULL){
		Data *p = data_manager.head->next;
		destory_data_node(data_manager.head);
		data_manager.head = p;
	}
}

template <typename Type>
bool MyList<Type>::add(Type *data)
{
	if(data_manager.head == NULL){
		data_manager.head = (Data *)calloc(1, sizeof(Data));
		if(data_manager.head == NULL){
			err("Data calloc failed");
			return false;
		}
		data_manager.head->node = data;
#ifdef DEBUG
		data_manager.count++;
#endif
		return true;
	}
	Data *p = data_manager.head;
	while(p->next != NULL){ 
		p = p->next;
	}

	Data *n = (Data *)calloc(1, sizeof(Data));
	if(n == NULL){
		err("Data calloc failed");
		return false;
	}
	n->node = data;
	p->next = n;
#ifdef DEBUG
		data_manager.count++;
#endif
	return true;
}

template <typename Type>
bool MyList<Type>::del(Type *data)
{
	if(data_manager.head == NULL){
		inf("data list is empty");
		return true; 
	}
	Data *p = data_manager.head;
	Data *prev = p;
	while(p != NULL){
		if(data == p->node){
			if(prev == p){
				data_manager.head = p->next;
			}else{
				prev->next = p->next;
			}
			destory_data_node(p);
#ifdef DEBUG
		data_manager.count--;
#endif
			return true;
		}
		prev = p;
		p = p->next;
	}
	return false;
}

template <typename Type>
Type *MyList<Type>::find(void *param)
{
	if(data_manager.compare == NULL){
		err("please register compare first");
		return NULL;
	}
	if(data_manager.head == NULL){
		inf("data list is empty");
		return NULL; 
	} 
	Data *p = data_manager.head;
	while(p){
		if(data_manager.compare(param, p->node)){
			inf("data exist");
			return p->node;
		}
		p = p->next;
	}
	return NULL;
}

template <typename Type>
void MyList<Type>::show()
{
#ifdef DEBUG
	dbg_t("data count: %d", data_manager.count);
#endif
}

template <typename Type>
void MyList<Type>::register_destory(bool (*destory)(Type *data))
{
	data_manager.destory = destory;
}

template <typename Type>
void MyList<Type>::register_compare(bool (*compare)(void *param, Type * data))
{
	data_manager.compare = compare;
}
#endif
