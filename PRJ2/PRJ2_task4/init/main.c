#include <common.h>
#include <asm.h>
#include <asm/unistd.h>
#include <os/loader.h>
#include <os/irq.h>
#include <os/sched.h>
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
#include <asm/regs.h>

extern long (*syscall[NUM_SYSCALLS])();
extern void ret_from_exception();

// Task info array
task_info_t tasks[TASK_MAXNUM];
uint16_t tasknum;

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
    jmptab[SD_WRITE]        = (long (*)())sd_write;
    jmptab[QEMU_LOGGING]    = (long (*)())qemu_logging;
    jmptab[SET_TIMER]       = (long (*)())set_timer;
    jmptab[READ_FDT]        = (long (*)())read_fdt;
    jmptab[MOVE_CURSOR]     = (long (*)())screen_move_cursor;
    jmptab[PRINT]           = (long (*)())printk;
    jmptab[YIELD]           = (long (*)())do_scheduler;
    jmptab[MUTEX_INIT]      = (long (*)())do_mutex_lock_init;
    jmptab[MUTEX_ACQ]       = (long (*)())do_mutex_lock_acquire;
    jmptab[MUTEX_RELEASE]   = (long (*)())do_mutex_lock_release;

    // TODO: [p2-task1] (S-core) initialize system call table.
    jmptab[WRITE]           = (long (*)())screen_write;
    jmptab[REFLUSH]         = (long (*)())screen_reflush;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    uint32_t offset = *((uint32_t *)(0x502001f4));
    task_info_t *taskinfo_ptr =  (task_info_t *)(0x52000000 + offset % SECTOR_SIZE);
	for(int i = 0; i < TASK_MAXNUM; i++, taskinfo_ptr++){
		tasks[i] = *taskinfo_ptr;
    }
	tasknum = *((uint16_t *)0x502001fe);
}

/************************************************************/
static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    if(!entry_point){
        printk("WARNING:entrypoint is NULL!!!!!");
        while(1){
            ;
        }
        return ;
    }
     /* TODO: [p2-task3] initialization of registers on kernel stack
      * HINT: sp, ra, sepc, sstatus
      * NOTE: To run the task in user mode, you should set corresponding bits
      *     of sstatus(SPP, SPIE, etc.).
      */
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    for(int i=0; i<32; i++){
        pt_regs->regs[i]=0;
    }
	pt_regs->sstatus = SR_SPIE;
	pt_regs->sepc = entry_point;
	pt_regs->regs[SP] = (reg_t)user_stack;
	pt_regs->regs[TP] = (reg_t)pcb;

    /* TODO: [p2-task1] set sp to simulate just returning from switch_to
     * NOTE: you should prepare a stack, and push some values to
     * simulate a callee-saved context.
     */
    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

    pt_switchto->regs[0] = (reg_t)ret_from_exception;//(reg_t)entry_point; //ra
    pt_switchto->regs[1] = (reg_t)pt_switchto;         //sp
    for(int i = 2; i < 14; i++){
        pt_switchto->regs[i] = 0;
    }

    pcb->kernel_sp = (reg_t)pt_switchto;
    pcb->user_sp = (reg_t)user_stack;

}

static void init_pcb(void)
{
    /* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
    char needed_task_name[][32] = {"print1", "print2", "fly", "timer", "sleep", "lock1", "lock2"};
    char fly_task_name[][32] = {"fly1", "fly2", "fly3", "fly4", "fly5"};
    //pcb[0]=pid0_pcb;
    //allocKernelPage(1);
    //allocUserPage(1);
    for (int i = 0; i < 7; i++){
        pcb[i].pid = i + 1;
        pcb[i].status = TASK_READY;
        pcb[i].total_running = 0;
		pcb[i].list.prev = NULL;
		pcb[i].list.next = NULL;
        pcb[i].list.pcb_ptr = (ptr_t)&pcb[i];

        pcb[i].time_chunk = TIMER_INTERVAL;
        init_pcb_stack(
            allocKernelPage(1),
            allocUserPage(1),
            load_task_img_name(needed_task_name[i]), 
            &pcb[i]
        );
        addQueue(&pcb[i].list, &ready_queue);
    }
    /* TODO: [p2-task1] remember to initialize 'current_running' */
    pid0_pcb.status = TASK_RUNNING;
    current_running = &pid0_pcb;
    process_id = pid0_pcb.pid;
}

static void init_syscall(void)
{
    // TODO: [p2-task3] initialize system call table.
	syscall[SYSCALL_SLEEP] 		= (long (*)())do_sleep;
	syscall[SYSCALL_YIELD] 		= (long (*)())do_scheduler;
	syscall[SYSCALL_WRITE] 		= (long (*)())screen_write;
	syscall[SYSCALL_CURSOR] 	= (long (*)())screen_move_cursor;
	syscall[SYSCALL_REFLUSH] 	= (long (*)())screen_reflush;
	syscall[SYSCALL_GET_TIMEBASE] 	= (long (*)())get_time_base;
	syscall[SYSCALL_GET_TICK] 	= (long (*)())get_ticks;
	syscall[SYSCALL_LOCK_INIT] 	= (long (*)())do_mutex_lock_init;
	syscall[SYSCALL_LOCK_ACQ] 	= (long (*)())do_mutex_lock_acquire;
	syscall[SYSCALL_LOCK_RELEASE]	= (long (*)())do_mutex_lock_release;
}

int main(void)
{
    // Init jump table provided by kernel and bios(ΦωΦ)
    init_jmptab();

    // Init task information (〃'▽'〃)
    init_task_info();

    // Init Process Control Blocks |•'-'•) ✧
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n");

    // Read CPU frequency (｡•ᴗ-)_
    time_base = bios_read_fdt(TIMEBASE);

    // Init lock mechanism o(´^｀)o
    init_locks();
    printk("> [INIT] Lock mechanism initialization succeeded.\n");

    // Init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    // Init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    // Init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");

    // TODO: [p2-task4] Setup timer interrupt and enable all interrupt globally
    // NOTE: The function of sstatus.sie is different from sie's
    bios_set_timer(get_ticks() + TIMER_INTERVAL);


    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        // If you do non-preemptive scheduling, it's used to surrender control
        //do_scheduler();

        // If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
        enable_preempt();
        asm volatile("wfi");
    }

    return 0;
}
