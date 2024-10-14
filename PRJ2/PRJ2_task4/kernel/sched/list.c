#include "type.h"
#include <os/list.h>
#include <os/sched.h>
/* 约定	单一节点的prev和next为NULL 空队列的prev和next指向头节点*/
void addQueue(list_node_t *listnode, list_head *queue)
{
	(queue->prev)->next = listnode;
	listnode->prev = queue->prev;
	listnode->next = queue;
	queue->prev = listnode;
}

void delNode(list_node_t *listnode)
{
	// need comments //
	if(!listnode || !listnode->prev || !listnode->next){
		return ;
	}
	listnode->prev->next = listnode->next;
	listnode->next->prev = listnode->prev;
	listnode->next = NULL;
	listnode->prev = NULL;
}

list_node_t *returnTop(list_head *queue)
{
	if(!queue || queue->prev == queue){
		return NULL;
	}
	return queue->next;
}

ptr_t getProcess()
{
	if(ready_queue.next != &ready_queue)
		return (ptr_t)((void *)ready_queue.next-16);
	else
		return (ptr_t)(&pid0_pcb);
}