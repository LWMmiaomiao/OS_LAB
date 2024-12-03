#include <common.h>
#include <asm.h>
#include <asm/unistd.h>
#include <asm/regs.h>
#include <os/smp.h>
#include <os/loader.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/exec.h>		//for p3-task1
#include <os/list.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/time.h>
#include <sys/syscall.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>
#include <os/page.h>
#include <pgtable.h>
#include <e1000.h>
#include <os/net.h>
#include <os/ioremap.h>

extern void ret_from_trap();

// Task info array
task_info_t tasks[TASK_MAXNUM];
uint64_t image_end_sec;
// char buf[50];

// int tasknum;


static void init_jmptab(void)
{
	volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    	jmptab[CONSOLE_PUTSTR]  = (volatile long (*)())port_write;
    	jmptab[CONSOLE_PUTCHAR] = (volatile long (*)())port_write_ch;
    	jmptab[CONSOLE_GETCHAR] = (volatile long (*)())port_read_ch;
    	jmptab[SD_READ]         = (volatile long (*)())sd_read;
    	jmptab[SD_WRITE]        = (volatile long (*)())sd_write;
    	jmptab[QEMU_LOGGING]    = (volatile long (*)())qemu_logging;
    	jmptab[SET_TIMER]       = (volatile long (*)())set_timer;
    	jmptab[READ_FDT]        = (volatile long (*)())read_fdt;
    	jmptab[MOVE_CURSOR]     = (volatile long (*)())screen_move_cursor;
    	jmptab[PRINT]           = (volatile long (*)())printk;
    	jmptab[YIELD]           = (volatile long (*)())do_scheduler;
    	jmptab[MUTEX_INIT]      = (volatile long (*)())do_mutex_lock_init;
    	jmptab[MUTEX_ACQ]       = (volatile long (*)())do_mutex_lock_acquire;
    	jmptab[MUTEX_RELEASE]   = (volatile long (*)())do_mutex_lock_release;

	// TODO: [p2-task1] (S-core) initialize system call table.
	jmptab[WRITE]		= (volatile long (*)())screen_write;
	jmptab[REFLUSH]		= (volatile long (*)())screen_reflush;

}

static void init_task_info(void)
{
	// TODO: [p1-task4] Init 'tasks' array via reading app-info sector
	// NOTE: You need to get some related arguments from bootblock first
	task_info_t * taskinfo_ptr = (task_info_t *)0xffffffc050200200;
	for(int i = 0; i < TASK_MAXNUM; i++, taskinfo_ptr++)
		tasks[i] = *taskinfo_ptr;
	// tasknum = *((int *)0x502001fe);
	for(int i = 0; i < TASK_MAXNUM && tasks[i].filename[0]!='\0'; i++)
		image_end_sec = NBYTES2SEC(tasks[i].offset + tasks[i].size) + 1;
}


/************************************************************/
// static void init_pcb_stack(
//     ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
//     pcb_t *pcb, int argc, char **argv)
// {
// 	// P3, pass parameter to the user stack
// 	ptr_t argv_base;
// 	ptr_t usp;
// 	argv_base = user_stack - sizeof(char *) * argc;
// 	user_stack = argv_base;
// 	usp = argv_base;
// 	for(int i = 0; i < argc; i++)
// 	{
// 		user_stack = user_stack - (strlen(argv[i]) + 1);
// 		strcpy((char *)user_stack,argv[i]);
// 		memcpy((uint8_t *)usp, (const uint8_t *)&user_stack, sizeof(char *));
// 		usp += sizeof(char *);
// 	}
// 	user_stack = user_stack & 0xffffffffffffff80;

// 	/* TODO: [p2-task3] initialization of registers on kernel stack
// 	* HINT: sp, ra, sepc, sstatus
// 	* NOTE: To run the task in user mode, you should set corresponding bits
// 	*     of sstatus(SPP, SPIE, etc.).
// 	*/
// 	regs_context_t *pt_regs = (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
	
// 	pt_regs->sstatus = SR_SPIE;		// return U-mode(SPP == 0) and enable interrupt gloablly(SPIE == 1)
// 	pt_regs->sepc = entry_point;		// jump to entrypoint using sret
// 	pt_regs->regs[SP] = user_stack;
// 	pt_regs->regs[TP] = (reg_t)pcb;
// 	pt_regs->regs[A0] = (reg_t)argc;
// 	pt_regs->regs[A1] = (reg_t)argv_base;


// 	/* TODO: [p2-task1] set sp to simulate just returning from switch_to
// 	* NOTE: you should prepare a stack, and push some values to
// 	* simulate a callee-saved context.
// 	*/
// 	switchto_context_t *pt_switchto = (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

// 	pcb->kernel_sp = (reg_t)pt_switchto;
// 	pcb->user_sp = (reg_t)user_stack;

// 	// for user process, jump to entrypoint by using sret
// 	pt_switchto->regs[0] = (reg_t)ret_from_trap;
// 	pt_switchto->regs[1] = pcb->kernel_sp;
// 	pt_switchto->regs[2] = 0;
// 	pt_switchto->regs[3] = 0;
// 	pt_switchto->regs[4] = 0;
// 	pt_switchto->regs[5] = 0;
// 	pt_switchto->regs[6] = 0;
// 	pt_switchto->regs[7] = 0;
// 	pt_switchto->regs[8] = 0;
// 	pt_switchto->regs[9] = 0;
// 	pt_switchto->regs[10] = 0;
// 	pt_switchto->regs[11] = 0;
// 	pt_switchto->regs[12] = 0;
// 	pt_switchto->regs[13] = 0;


// }



// int add_new_task(char * str, int argc, char *argv[], int pid)
// {
// 	ptr_t entrypoint;
// 	ptr_t kernel_stack,usr_stack;

// 	if((entrypoint = load_task_img_by_name(str)) != 0)
// 	{
// 		kernel_stack = allocKernelStack(1);
// 		usr_stack = allocUserStack(4);
// 		init_pcb_stack(kernel_stack, usr_stack, entrypoint, &pcb[pid-1],argc,argv);
// 		pcb[pid-1].status = TASK_READY;
// 		return 0;
// 	}
// 	else  
// 		return -1;
// }


static void init_pcb(void)
{
	/* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
	int i;
	int current_cpuid = get_current_cpu_id();
	// initialize pcb array
	for(i = 0; i < NUM_MAX_TASK; i++)
	{
		pcb[i].pid = i + 1;
		pcb[i].list.prev = NULL;
		pcb[i].list.next = NULL;
		pcb[i].list.pcb_ptr = (ptr_t)&pcb[i];
		pcb[i].wait_list.next = &pcb[i].wait_list;
		pcb[i].wait_list.prev = &pcb[i].wait_list;
		pcb[i].status = TASK_EXITED;			// useless?
		pcb[i].current_core_id = NO_CORE;

		pcb[i].kernel_stack_base = allocPage_pin(0, 0, PINNED);
		pcb[i].user_stack_base = 0xf0000f000;
		pcb[i].pagedir = NULL;
		pcb[i].next_stack_base = pcb[i].user_stack_base + 2 * PAGE_SIZE;
	}

	/* TODO: [p2-task1] remember to initialize 'current_running' */

	pid0_pcb[0].status = TASK_RUNNING;
	pid0_pcb[0].current_core_id = CORE_ZERO;
	current_running = &pid0_pcb[current_cpuid];		// current running is kernel
	process_id[current_cpuid] = pid0_pcb[current_cpuid].pid;

	pid0_pcb[0].kernel_sp = allocPage_pin(0, 0, PINNED) + PAGE_SIZE;
	pid0_pcb[0].user_sp = pid0_pcb[0].kernel_sp;
	pid0_pcb[0].pagedir = initPgtable(0);
	pid0_pcb[1].kernel_sp = allocPage_pin(0, 0, PINNED) + PAGE_SIZE;
	pid0_pcb[1].user_sp = pid0_pcb[1].kernel_sp;
	pid0_pcb[1].pagedir = initPgtable(0);

	// load shell
	int shell_argc = 2;
	char *shell_argv[shell_argc];
	shell_argv[0] = "0x3";
	shell_argv[1] = "shell";
	do_taskset(shell_argc, shell_argv);

}

static void init_syscall(void)
{
	// TODO: [p2-task3] initialize system call table.
	syscall[SYSCALL_SLEEP] 		= (long (*)())do_sleep;
	syscall[SYSCALL_YIELD] 		= (long (*)())do_scheduler;
	syscall[SYSCALL_TASKSET]	= (long (*)())do_taskset;
	syscall[SYSCALL_WRITE] 		= (long (*)())screen_write;
	syscall[SYSCALL_CURSOR] 	= (long (*)())screen_move_cursor;
	syscall[SYSCALL_REFLUSH] 	= (long (*)())screen_reflush;
	syscall[SYSCALL_GET_TIMEBASE] 	= (long (*)())get_time_base;
	syscall[SYSCALL_GET_TICK] 	= (long (*)())get_ticks;
	syscall[SYSCALL_LOCK_INIT] 	= (long (*)())do_mutex_lock_init;
	syscall[SYSCALL_LOCK_ACQ] 	= (long (*)())do_mutex_lock_acquire;
	syscall[SYSCALL_LOCK_RELEASE]	= (long (*)())do_mutex_lock_release;
	syscall[SYSCALL_PUTCHAR]	= (long (*)())screen_putchar;
	syscall[SYSCALL_GETCHAR]	= (long (*)())bios_getchar;
	syscall[SYSCALL_PS]		= (long (*)())do_process_show;
	syscall[SYSCALL_EXEC] 		= (long (*)())do_exec;
	syscall[SYSCALL_EXIT]		= (long (*)())do_exit;
	syscall[SYSCALL_CLEAR]		= (long (*)())screen_clear;
	syscall[SYSCALL_KILL]		= (long (*)())do_kill;
	syscall[SYSCALL_GETPID]		= (long (*)())do_getpid;
	syscall[SYSCALL_WAITPID] 	= (long (*)())do_waitpid;
	syscall[SYSCALL_BARR_INIT]	= (long (*)())do_barrier_init;
	syscall[SYSCALL_BARR_WAIT]	= (long (*)())do_barrier_wait;
	syscall[SYSCALL_BARR_DESTROY]	= (long (*)())do_barrier_destroy;
	syscall[SYSCALL_COND_INIT]	= (long (*)())do_condition_init;
	syscall[SYSCALL_COND_WAIT]	= (long (*)())do_condition_wait;
	syscall[SYSCALL_COND_SIGNAL]	= (long (*)())do_condition_signal;
	syscall[SYSCALL_COND_BROADCAST]	= (long (*)())do_condition_broadcast;
	syscall[SYSCALL_COND_DESTROY]	= (long (*)())do_condition_destroy;
	syscall[SYSCALL_MBOX_OPEN]	= (long (*)())do_mbox_open;
	syscall[SYSCALL_MBOX_CLOSE]	= (long (*)())do_mbox_close;
	syscall[SYSCALL_MBOX_SEND]	= (long (*)())do_mbox_send;
	syscall[SYSCALL_MBOX_RECV]	= (long (*)())do_mbox_recv;
	syscall[SYSCALL_THREAD_CREATE] = (long (*)())thread_create; // p4 task4
	syscall[SYSCALL_THREAD_YIELD] = (long (*)())do_waitpid;
	syscall[SYSCALL_SHM_GET] = (long (*)())shm_page_get; // p4 task5
	syscall[SYSCALL_SHM_DT] = (long (*)())shm_page_dt;
	syscall[SYSCALL_NET_SEND] = (long (*)())do_net_send;
	syscall[SYSCALL_NET_RECV] = (long (*)())do_net_recv;

}

/************************************************************/

/*
 * Once a CPU core calls this function,
 * it will stop executing!
 */
static void kernel_brake(void)
{
    disable_interrupt();
    while (1)
        __asm__ volatile("wfi");
}

int main(void)
{
	if(get_current_cpu_id() == 0)
	{
		// Init jump table provided by kernel and bios(ΦωΦ)
		init_jmptab();

		// Init task information (〃'▽'〃)
		init_task_info();

		// Output 'Hello OS!'
		bios_putstr("Hello OS!\n\r");
		
		// Init Physical memory allocator
		initkmem();
		bios_putstr("[INIT] Memory initialization succeeded.\n\r");

		// Init Process Control Blocks |•'-'•) ✧
		init_pcb();
		printk("> [INIT] PCB initialization succeeded.\n");

		// Read Flatten Device Tree (｡•ᴗ-)_
		time_base = bios_read_fdt(TIMEBASE);
		e1000 = (volatile uint8_t *)bios_read_fdt(ETHERNET_ADDR);
		uint64_t plic_addr = bios_read_fdt(PLIC_ADDR);
		uint32_t nr_irqs = (uint32_t)bios_read_fdt(NR_IRQS);
		printk("> [INIT] e1000: %lx, plic_addr: %lx, nr_irqs: %lx.\n", e1000, plic_addr, nr_irqs);
		
		// IOremap
		plic_addr = (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000 * NORMAL_PAGE_SIZE);
		e1000 = (uint8_t *)ioremap((uint64_t)e1000, 8 * NORMAL_PAGE_SIZE);
		printk("> [INIT] IOremap initialization succeeded.\n");

		// Init lock mechanism o(´^｀)o
		init_ipc();
		printk("> [INIT] Lock mechanism initialization succeeded.\n");

		// Init interrupt (^_^)
		init_trap();
		printk("> [INIT] Interrupt processing initialization succeeded.\n");

		// Init system call table (0_0)
		init_syscall();
		printk("> [INIT] System call initialized successfully.\n");

		// Init screen (QAQ)
		init_screen();
		printk("> [INIT] SCREEN initialization succeeded.\n");

		/*
		* Just start kernel with VM and print this string
		* in the first part of task 1 of project 4.
		* NOTE: if you use SMP, then every CPU core should call
		*  `kernel_brake()` to stop executing!
		*/
		printk("> [INIT] CPU #%u has entered kernel with VM!\n",
			(unsigned int)get_current_cpu_id());
		// TODO: [p4-task1 cont.] remove the brake and continue to start user processes.
		// kernel_brake();

		wakeup_other_hart();

		printl("core 0\n");

	}
	else
	{
		smp_init();
		unmapBoot();
		printl("core 1\n");
	}

	/*
	do_sleep(2);

	screen_clear();

	getTask();

	allocReadyProcess();

	do_sleep(2);

	screen_clear();
	*/


	// TODO: [p2-task4] Setup timer interrupt and 
	// enable all interrupt globally
	// NOTE: The function of sstatus.sie is different from sie's

	//enable_interrupt();

	bios_set_timer(get_ticks() + TIMER_INTERVAL);


	// Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
	while (1)
	{
		// If you do non-preemptive scheduling, it's used to surrender control
		// do_scheduler();

		// If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
		enable_preempt();
		asm volatile("wfi");
	}

	return 0;
}
