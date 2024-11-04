#include <os/list.h>
#include <os/sched.h>
#include <printk.h>
void addToQueue(list_node_t * listnode, list_head * queue)
{
	(queue->prev)->next = listnode;
	listnode->prev = queue->prev;
	listnode->next = queue;
	queue->prev = listnode;
}

void deleteNode(list_node_t * listnode)
{
	if(listnode->next && listnode->prev)
	{
		listnode->prev->next = listnode->next;
		listnode->next->prev = listnode->prev;
	
		// note that the pointer is NULL
		listnode->next = NULL;
		listnode->prev = NULL;
	}
	else
		printl("ERROR: In function deleteNode, the list is not in a queue\n");
}

void allocReadyProcess()
{
	int i;
	for(i=0; i < NUM_MAX_TASK; i++)
	{
		if(pcb[i].status == TASK_READY)
			addToQueue(&pcb[i].list,&ready_queue);
	}
}

ptr_t getProcess()
{
	int ready_cpu_id = get_current_cpu_id();
    list_node_t *p = ready_queue.next;
    pcb_t *tmp;
    while(p != &ready_queue){
        // 就绪队列剩余非空
        tmp = (pcb_t *)FIND_PCB(p);
        if(tmp->cpu_mask & (ready_cpu_id + 1)){ // 掩码按位与
            deleteNode(p);
            return (ptr_t)tmp;
        }
        p = p->next;
    }
    return (ptr_t)(&pid0_pcb[get_current_cpu_id()]);



	if(ready_queue.next != &ready_queue)
		return (ptr_t)FIND_PCB(ready_queue.next);
	else
		return (ptr_t)(&pid0_pcb[get_current_cpu_id()]);
}

void freeQueueToReady(list_head * head)
{
	while(head->next != head)
	{
		do_unblock(head->next);
	}
}