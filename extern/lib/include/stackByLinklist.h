/*************************************************************************
	>c++ File Name: stackBylinklist.h
	>c++ Created Time: 2020年05月12日 星期二 15时16分39秒
 ************************************************************************/

typedef struct stackNode{
	void *data;
	struct stackNode* next;
}*pStackNode;

typedef struct stackLinklist{
	unsigned int max;
	unsigned int count;
	pStackNode top;
	void *(*pop)(struct stackLinklist *stack);
	int (*push)(struct stackLinklist *stack, void *data);
	int (*isEmpty)(struct stackLinklist *stack);
	int (*isFull)(struct stackLinklist *stack);
}*pStackLinklist;

pStackLinklist createStackLinklist(unsigned int max);
void destoryStackLinklist(pStackLinklist *stack);
