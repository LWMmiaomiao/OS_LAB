#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
//#include <unistd.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .time_chunk = TIMER_INTERVAL
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* global process id */
pid_t process_id = 1;
long long pid_time = 0;

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs

    /************************************************************/
    /* Do not touch this comment. Reserved for future projects. */
    /************************************************************/
    pid_time++;
    // TODO: [p2-task1] Modify the current_running pointer.
    pcb_t *prev_running = current_running;
    if(prev_running->status == TASK_RUNNING){
        prev_running->status = TASK_READY;
        if(prev_running != &pid0_pcb){
			addQueue(&current_running->list, &ready_queue);
        }
    }

    /*replace*/
    current_running->total_running += current_running->time_chunk;
    
    current_running = (pcb_t *)getProcess();
    current_running->status = TASK_RUNNING;
	if(current_running->pid != 0){
		delNode(ready_queue.next);
    }
    process_id = current_running->pid;
    // TODO: [p2-task1] switch_to current_running
    switch_to(prev_running, current_running);
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: [p2-task2] block the pcb task into the block queue
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
}
