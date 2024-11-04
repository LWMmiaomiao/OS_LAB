#include <os/kernel.h>
#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/exec.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack[NR_CPUS] = {INIT_KERNEL_STACK - PAGE_SIZE, INIT_KERNEL_STACK + PAGE_SIZE};
pcb_t pid0_pcb[NR_CPUS] = {
	{.pid = 0,.kernel_sp = INIT_KERNEL_STACK - PAGE_SIZE,.user_sp = INIT_KERNEL_STACK - PAGE_SIZE,
	.run_cpu_id = 0, .cpu_mask = 0x01},
	{.pid = 0,.kernel_sp = INIT_KERNEL_STACK + PAGE_SIZE,.user_sp = INIT_KERNEL_STACK + PAGE_SIZE,
	.run_cpu_id = 1, .cpu_mask = 0x02}
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* global process id */
pid_t process_id[NR_CPUS];


void do_scheduler(void)
{
	// TODO: [p2-task3] Check sleep queue to wake up PCBs
	check_sleeping();

	/************************************************************/
	/* Do not touch this comment. Reserved for future projects. */
	/************************************************************/

	// TODO: [p2-task1] Modify the current_running pointer.
	int current_cpuid = get_current_cpu_id();
	pcb_t * prev_process = current_running;
	prev_process->run_cpu_id = -1;
	if(prev_process->status == TASK_RUNNING)
	{
		prev_process->status = TASK_READY;

		// Round Robin
		if(prev_process != &pid0_pcb[current_cpuid])
		{
			addToQueue(&prev_process->list, &ready_queue);
		}
	}
	

	current_running = (pcb_t *)getProcess();
	process_id[current_cpuid] = current_running->pid;
	current_running->status = TASK_RUNNING;
	current_running->run_cpu_id = current_cpuid;

	bios_set_timer(get_ticks() + TIMER_INTERVAL);	// set timer interrupt

	// TODO: [p2-task1] switch_to current_running
	switch_to(prev_process,current_running);
}

void do_sleep(uint32_t sleep_time)
{
	// TODO: [p2-task3] sleep(seconds)
	// NOTE: you can assume: 1 second = 1 `timebase` ticks
	// 1. block the current_running
	// 2. set the wake up time for the blocked task
	// 3. reschedule because the current_running is blocked.
	if(current_running != &pid0_pcb[get_current_cpu_id()])
	{
		current_running->status = TASK_BLOCKED;
		current_running->wakeup_time = get_timer() + sleep_time;
		addToQueue(&current_running->list, &sleep_queue);
		do_scheduler();
	}
	else
	{
		latency(sleep_time);		// in kernel process, call latency
	}

}

void do_block(list_node_t *pcb_node, list_head *queue)
{
	// TODO: [p2-task2] block the pcb task into the block queue
	current_running->status = TASK_BLOCKED;
	if(current_running != &pid0_pcb[get_current_cpu_id()])
		addToQueue(pcb_node, queue);

	do_scheduler();
}

void do_unblock(list_node_t *pcb_node)
{
	// TODO: [p2-task2] unblock the `pcb` from the block queue
	pcb_t * pcb_unblock = FIND_PCB(pcb_node);
	deleteNode(pcb_node);
	pcb_unblock->status = TASK_READY;
	addToQueue(pcb_node, &ready_queue);
}

void do_process_show()
{
    static char *state_str[4]={
        "BLOCKED","RUNNING","READY","EXITED"
    };
    printk("[Process Table]:\n");
    for(int i = 0, j = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].status == TASK_EXITED){
            continue;
		}
        else if(pcb[i].status == TASK_RUNNING){
            printk("[%d] PID : %d  STATUS : %s mask : 0x%x Running on core %d\n", j, pcb[i].pid, state_str[pcb[i].status], pcb[i].cpu_mask, pcb[i].run_cpu_id);
			j++;
		}
		else{
            printk("[%d] PID : %d  STATUS : %s mask : 0x%x\n", j, pcb[i].pid, state_str[pcb[i].status], pcb[i].cpu_mask);
			j++;
		}
		printl("[%d] PID : %d  STATUS : %s mask : 0x%x\n", j, pcb[i].pid, state_str[pcb[i].status], pcb[i].cpu_mask);
    }
}

pid_t do_getpid()
{
	return current_running->pid;
	//return current_running[cpu_id]->pid;
}

pid_t do_exec(char *name, int argc, char **argv)
{
	int i;
	pid_t pid = 0;
	for(i = 0; i < NUM_MAX_TASK; i++)
	{
		if(pcb[i].status == TASK_EXITED)
		{		
			pid = i + 1;	
			if(add_new_task(name, argc, argv, pid) == -1)
			{
				printl("Error: In function do_exec, cannot load task called %s\n",name);
				return 0;
			}
			pcb[pid - 1].cpu_mask = current_running->cpu_mask;
			printl("TEST POINT: name : %s pid : %d cpu_mask : %d curpid : %d\n",name, pid, pcb[pid].cpu_mask, current_running->pid);
			addToQueue(&pcb[i].list,&ready_queue);
			break;
		}
	}
	return pid;
}

// 回收内存
void do_exit(void)
{
	current_running->status = TASK_EXITED;
	freeQueueToReady(&current_running->wait_list);
	do_scheduler();
}

int do_kill(pid_t pid)
{
	if(pcb[pid - 1].status != TASK_EXITED)
	{
		deleteNode(&pcb[pid - 1].list);
		pcb[pid - 1].status = TASK_EXITED;
		freeQueueToReady(&pcb[pid - 1].wait_list);
		// que :释放锁的逻辑需要改变，应该是每个锁持有进程的pid
		do_mutex_lock_release_bypid(pid);
		// if(pcb[pid-1].mlock_idx != -1)
		// 	do_mutex_lock_release(pcb[pid-1].mlock_idx);
		if(pcb[pid-1].mbox_idx != -1)
			do_mbox_close(pcb[pid-1].mbox_idx);
		return 1;
	}
	return 0;
}

int do_kill_itself(pid_t shell_pid)
{
	for(int i = 0; i < NUM_MAX_TASK; i++){
		if(pcb[i].status != TASK_EXITED && pcb[i].pid != shell_pid){
			do_kill(pcb[i].pid);	// kill自身时先kill其他进程
		}
	}
	do_exit();
	return 0;
}

int do_waitpid(pid_t pid)
{
	if(pcb[pid - 1].status != TASK_EXITED)
	{
		do_block(&current_running->list, &pcb[pid - 1].wait_list);
		return pid;
	}
	return 0;
}

void do_taskset_pid(int mask, pid_t setpid){
	for(int i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].status != TASK_EXITED && pcb[i].pid == setpid){
            pcb[i].cpu_mask = mask;
        	return ;
        }
    }
    printk("WARNING:NO TASK WITH PID %d TO SET MASK %d\n", setpid, mask);
}

void do_taskset_name(int mask, char *name){
	pid_t setpid = do_exec(name, 1, (char **)&name);
	printk("taskset name : %s mask : %d\n", name, mask);
	for(int i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].status != TASK_EXITED && pcb[i].pid == setpid){
            pcb[i].cpu_mask = mask;
        	return ;
        }
    }
    printk("WARNING:NO TASK WITH NAME %s TO SET MASK %d\n", name, mask);
}