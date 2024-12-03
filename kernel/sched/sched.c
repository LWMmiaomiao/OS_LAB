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
#include <pgtable.h>
#include <csr.h>
#include <os/page.h>
#include <os/loader.h>

extern void ret_from_trap();

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack[NR_CPUS] = {INIT_KERNEL_STACK - PAGE_SIZE, INIT_KERNEL_STACK + PAGE_SIZE};
pcb_t pid0_pcb[NR_CPUS] = {
	{.pid = 0,.kernel_sp = INIT_KERNEL_STACK - PAGE_SIZE,.user_sp = INIT_KERNEL_STACK - PAGE_SIZE,
	 .core_mask = MASK_ZERO},
	{.pid = 0,.kernel_sp = INIT_KERNEL_STACK + PAGE_SIZE,.user_sp = INIT_KERNEL_STACK + PAGE_SIZE,
	 .core_mask = MASK_ONE}
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
	list_head * queue;
	if(current_running->status == TASK_RUNNING)
	{
		current_running->status = TASK_READY;

		// Round Robin
		if(current_running != &pid0_pcb[current_cpuid])
		{
			addToQueue(&current_running->list, &ready_queue);
		}
	}

	current_running->current_core_id = NO_CORE;	

	// 根据掩码选择下一个进程
	queue = &ready_queue;
	//current_running = (pcb_t *)getReadyProcess(queue);
	while(1)
	{
		current_running = (pcb_t *)getReadyProcess(queue);
		// printl("0x%x core: %d pid: %d\n",current_running,current_cpuid,process_id[current_cpuid]);
		if(current_running->core_mask & (1 << current_cpuid))
			break;
		queue = queue->next;
		if(queue->next == &ready_queue)
		{
			current_running = &pid0_pcb[current_cpuid];
			break;
		}
	}

	current_running->current_core_id = current_cpuid;

	process_id[current_cpuid] = current_running->pid;
	current_running->status = TASK_RUNNING;
	if(process_id[current_cpuid] != 0)
		deleteNode(&current_running->list);

	// add p4 task1
	set_satp(SATP_MODE_SV39, current_running->pid,
		 kva2pa((uintptr_t)current_running->pagedir) >>
			 NORMAL_PAGE_SHIFT);
	local_flush_tlb_all();
	local_flush_icache_all();

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
	int i;
	int j = 0;
	int has_process = 0;
	for(i = 0; i < NUM_MAX_TASK; i++)
	{
		if(pcb[i].status != TASK_EXITED)
		{
			if(has_process == 0)
			{
				printk("[Process Table]:\n");
				has_process = 1;
			}
			printk("[%d] PID: %d ",j,pcb[i].pid);
			j++;
			if(pcb[i].status == TASK_RUNNING)
				printk("STATUS: %s ","TASK_RUNNING");
			else if(pcb[i].status == TASK_BLOCKED)
				printk("STATUS: %s ","TASK_BLOCKED");
			else if(pcb[i].status == TASK_READY)
				printk("STATUS: %s ","TASK_READY");

			printk("MASK: 0x%x",pcb[i].core_mask);

			if(pcb[i].current_core_id == NO_CORE)
				printk("\n");
			else
				printk(" Core: %d\n",pcb[i].current_core_id);
		}
	}
	if(has_process == 0)
	{
		printk("Huh? There is no process?");
	}
}

pid_t do_getpid()
{
	return current_running->pid;
}

pid_t do_exec(char *name, int argc, char **argv)
{
	// int i;
	// pid_t pid = -1;
	// for(i = 0; i < NUM_MAX_TASK; i++)
	// {
	// 	if(pcb[i].status == TASK_EXITED)
	// 	{		
	// 		pid = i + 1;	
	// 		if(add_new_task(name,argc,argv,pid) == -1)
	// 		{
	// 			printl("Error: In function do_exec, cannot load task called %s\n",name);
	// 			return -1;
	// 		}
	// 		addToQueue(&pcb[i].list,&ready_queue);
	// 		pcb[i].core_mask = current_running->core_mask;
	// 		break;
	// 	}
	// }
	// return pid;	// 修改：错误返回-1

	printl("exec name is %s.", name);
	

	uint64_t entrypoint = getEntrypoint(name);
	if (entrypoint == 0) {
		printk("Can not find %s!\n", name);
		return 0;
	}
	int pcbidx = -1;
	for (int i = 0; i < NUM_MAX_TASK; ++i) {
		if (pcb[i].status == TASK_EXITED) {
			pcbidx = i;
			break;
		}
		else {
			printl("pcb[i] %d not usabel\n", i);
		}
	}
	assert(pcbidx != -1);
	pcb[pcbidx].pid = pcbidx + 1;
	pcb[pcbidx].pagedir = initPgtable(pcb[pcbidx].pid);
	from_name_load_task_img(name, &pcb[pcbidx]);
	// TODO: fix init_pcb_stack for access for user stack
	init_pcb_stack(pcb[pcbidx].kernel_stack_base + PAGE_SIZE,
		       pcb[pcbidx].user_stack_base + PAGE_SIZE, entrypoint,
		       pcb + pcbidx, argc, argv);
	pcb[pcbidx].status = TASK_READY;
	pcb[pcbidx].cursor_x = 0;
	pcb[pcbidx].cursor_y = 0;
	pcb[pcbidx].wakeup_time = 0;
	pcb[pcbidx].core_mask = current_running->core_mask;

	addToQueue(&pcb[pcbidx].list,&ready_queue);
	return pcb[pcbidx].pid;
}

// 回收内存
void do_exit(void)
{
	current_running->status = TASK_EXITED;
	freeQueueToReady(&current_running->wait_list);
	// add p4
	// 清除线程
	current_running->cursor_x = current_running->cursor_y = 0;
	current_running->kernel_sp = current_running->kernel_stack_base + PAGE_SIZE;
	current_running->user_sp = current_running->user_stack_base + PAGE_SIZE;
	current_running->user_stack_base = 0xf0000f000;
	unmapPageDir(current_running->pid);
	current_running->next_stack_base =
		current_running->user_stack_base + 2 * PAGE_SIZE;
	do_scheduler();
}

int do_kill(pid_t pid)
{
	if(pcb[pid - 1].status != TASK_EXITED)
	{
		deleteNode(&pcb[pid - 1].list);
		pcb[pid - 1].status = TASK_EXITED;
		freeQueueToReady(&pcb[pid - 1].wait_list);
		// 多把锁
		for(int i = 0; i < LOCK_NUM; i++)
		{
			if(pcb[pid - 1].mlock_table[i] == 1)
				do_mutex_lock_release(i);
		}
		for(int i = 0; i < MBOX_NUM; i++)
		{
			if(pcb[pid - 1].mbox_table[i] == 1)
				do_mbox_close(i);
		}
	}
	return 0;
}

int do_waitpid(pid_t pid)
{
	if(pid > 0 && pid <= NUM_MAX_TASK)
	{
		if(pcb[pid - 1].status != TASK_EXITED)
		{
			do_block(&current_running->list, &pcb[pid - 1].wait_list);
			return pid;
		}
	}
	return 0;
}

// add p4 tak1
pcb_t *pid2pcb(int pid)
{
	int ret = 0;
	for (int i = 0; i < NUM_MAX_TASK; ++i) {
		if (pid == pcb[i].pid) {
			ret = i;
		}
	}
	return &pcb[ret];
}

void init_pcb_stack(ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
		    pcb_t *pcb, int argc, char *argv[])
{
	// [p3-task1] set user stack for argv
	ptr_t vUserSp = user_stack;
	ptr_t pUserSp = allocPage_pin(pcb->pid, 0, PINNED);
	map_page(vUserSp - PAGE_SIZE, kva2pa((uintptr_t)pUserSp), pcb->pagedir,
		 pcb->pid);
	pUserSp += PAGE_SIZE;
	char **pArgvStack = (char **)(pUserSp - 8 * (argc + 1));
	char **vArgvStack = (char **)(vUserSp - 8 * (argc + 1));
	pUserSp = (ptr_t)pArgvStack;
	vUserSp = (ptr_t)vArgvStack;
	for (int i = 0; i < argc; ++i) {
		vUserSp -= strlen(argv[i]) + 1;
		pUserSp -= strlen(argv[i]) + 1;
		strcpy((char *)pUserSp, argv[i]);
		pArgvStack[i] = (char *)vUserSp;
	}
	pArgvStack[argc] = NULL;

	memset((void *)(pUserSp - (pUserSp % 16)), 0, pUserSp % 16);

	pUserSp -= (pUserSp % 16);
	vUserSp -= (vUserSp % 16);
	pcb->user_sp = vUserSp;
	/* TODO: [p2-task3] initialization of registers on kernel stack
      * HINT: sp, ra, sepc, sstatus
      * NOTE: To run the task in user mode, you should set corresponding bits
      *     of sstatus(SPP, SPIE, etc.).
      */
	regs_context_t *pt_regs =
		(regs_context_t *)(kernel_stack - sizeof(regs_context_t));
	for (int i = 0; i < 32; ++i)
		pt_regs->regs[i] = 0;
	pt_regs->regs[2] = (reg_t)pcb->user_sp; // sp
	pt_regs->regs[4] = (reg_t)pcb; // tp
	pt_regs->regs[1] = (reg_t)(entry_point + 2);
	pt_regs->regs[10] = (reg_t)argc;
	pt_regs->regs[11] = (reg_t)vArgvStack;
	// When a trap is taken, SPP is set to 0 if the trap originated from user mode, or 1 otherwise.
	pt_regs->sstatus = ((reg_t)SR_SPIE & (reg_t)~SR_SPP) | (reg_t)SR_SUM;
	pt_regs->sepc = (reg_t)entry_point;
	pt_regs->stval = (reg_t)0;
	pt_regs->scause = (reg_t)0;
	/* TODO: [p2-task1] set sp to simulate just returning from switch_to
     * NOTE: you should prepare a stack, and push some values to
     * simulate a callee-saved context.
     */
	switchto_context_t *pt_switchto =
		(switchto_context_t *)((ptr_t)pt_regs -
				       sizeof(switchto_context_t));
	for (int i = 0; i < 14; ++i)
		pt_switchto->regs[i] = (reg_t)0;
	// [p2-task1]
	// pt_switchto->regs[0] = (reg_t)entry_point;         // ra
	// [p2-task3]
	// pt_switchto->regs[0] = (reg_t)ret_from_exception;   // ra
	// [p3-task5]
	pt_switchto->regs[0] = (reg_t)ret_from_trap; // ra
	pt_switchto->regs[1] = (reg_t)pt_switchto; // sp

	pcb->kernel_sp = (reg_t)pt_switchto;
}

// add p4 task4
void init_pthread_stack(ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
			pcb_t *pcb, void *arg)
{
	// [p3-task1] set user stack for argv
	ptr_t vUserSp = user_stack;
	ptr_t pUserSp = allocPage_pin(pcb->pid, 0, PINNED);
	map_page(vUserSp - PAGE_SIZE, kva2pa((uintptr_t)pUserSp), pcb->pagedir,
		 pcb->pid);
	pcb->user_sp = vUserSp;
	/* TODO: [p2-task3] initialization of registers on kernel stack
      * HINT: sp, ra, sepc, sstatus
      * NOTE: To run the task in user mode, you should set corresponding bits
      *     of sstatus(SPP, SPIE, etc.).
      */
	regs_context_t *pt_regs =
		(regs_context_t *)(kernel_stack - sizeof(regs_context_t));
	for (int i = 0; i < 32; ++i)
		pt_regs->regs[i] = 0;
	pt_regs->regs[2] = (reg_t)pcb->user_sp; // sp
	pt_regs->regs[4] = (reg_t)pcb; // tp
	pt_regs->regs[1] = (reg_t)(0x10000 + 2);
	pt_regs->regs[10] = (reg_t)arg;
	// When a trap is taken, SPP is set to 0 if the trap originated from user mode, or 1 otherwise.
	pt_regs->sstatus = ((reg_t)SR_SPIE & (reg_t)~SR_SPP) | (reg_t)SR_SUM;
	pt_regs->sepc = (reg_t)entry_point;
	pt_regs->stval = (reg_t)0;
	pt_regs->scause = (reg_t)0;
	/* TODO: [p2-task1] set sp to simulate just returning from switch_to
     * NOTE: you should prepare a stack, and push some values to
     * simulate a callee-saved context.
     */
	switchto_context_t *pt_switchto =
		(switchto_context_t *)((ptr_t)pt_regs -
				       sizeof(switchto_context_t));
	for (int i = 0; i < 14; ++i)
		pt_switchto->regs[i] = (reg_t)0;

	pt_switchto->regs[0] = (reg_t)ret_from_trap; // ra
	pt_switchto->regs[1] = (reg_t)pt_switchto; // sp
	pcb->kernel_sp = (reg_t)pt_switchto;
}
int thread_create(int *tidptr, long entrypoint, void *arg)
{
	int pcbidx = -1;
	for (int i = 0; i < NUM_MAX_TASK; ++i) {
		if (pcb[i].status == TASK_EXITED) {
			pcbidx = i;
			break;
		}
	}
	printl("entry thread creat, point %ld\n", entrypoint);
	assert(pcbidx != -1);
	pcb[pcbidx].pid = pcbidx + 1;
	pcb[pcbidx].pagedir = current_running->pagedir;
	pcb[pcbidx].user_stack_base = current_running->next_stack_base;
	current_running->next_stack_base += 2 * PAGE_SIZE;
	init_pthread_stack(pcb[pcbidx].kernel_stack_base + PAGE_SIZE,
			   pcb[pcbidx].user_stack_base + PAGE_SIZE, entrypoint,
			   pcb + pcbidx, arg);
	pcb[pcbidx].status = TASK_READY;
	pcb[pcbidx].cursor_x = 0;
	pcb[pcbidx].cursor_y = 0;
	// pcb[pcbidx].tid = 0;
	pcb[pcbidx].wakeup_time = 0;
	pcb[pcbidx].core_mask = current_running->core_mask;
	addToQueue(&pcb[pcbidx].list,&ready_queue);
	*tidptr = pcb[pcbidx].pid;
	printl("return thread creat, id %d\n", *tidptr);
	return 0;
}