#include "type.h"
#include <os/list.h>
#include <os/sched.h>
void addQueue(list_node_t * listnode, list_head * queue)
{
	(queue->prev)->next = listnode;
	listnode->prev = queue->prev;
	listnode->next = queue;
	queue->prev = listnode;
}

void delNode(list_node_t * listnode)
{
	// need comments //
	listnode->prev->next = listnode->next;
	listnode->next->prev = listnode->prev;
	listnode->next = NULL;
	listnode->prev = NULL;
}

ptr_t getProcess()
{
	if(ready_queue.next != &ready_queue)
		return (ptr_t)FIND_PCB(ready_queue.next);
	else
		return (ptr_t)(&pid0_pcb);
}