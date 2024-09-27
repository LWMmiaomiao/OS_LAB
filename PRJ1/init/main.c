#include <common.h>
#include <asm.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <os/batch.h>
#include <type.h>

#define VERSION_BUF 50

int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];
uint16_t tasknum;
static inline void local_flush_icache_all(void)
{
  asm volatile ("fence.i" ::: "memory");
}
// Task info array
task_info_t tasks[TASK_MAXNUM];

static int bss_check(void)// 返回1表示缓冲区已清零
{
    for (int i = 0; i < VERSION_BUF; ++i)
    {
        if (buf[i] != 0)
        {
            return 0;
        }
    }
    return 1;
}

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    uint32_t offset = *((uint32_t *)(0x502001f4));
	task_info_t *taskinfo_ptr = (task_info_t *)(void *)(0x52000000 + offset % SECTOR_SIZE);
	for(int i = 0; i < TASK_MAXNUM; i++, taskinfo_ptr++){
		tasks[i] = *taskinfo_ptr;
    }
	tasknum = *((uint16_t *)0x502001fe);
}

static void clearbuf(void){
    for(int cle = 0; cle < VERSION_BUF; cle++){
        buf[cle] = 0;
    }
}

/************************************************************/
/* Do not touch this comment. Reserved for future projects. */
/************************************************************/

int main(void)
{
    // Check whether .bss section is set to zero
    int check = bss_check();

    // Init jump table provided by kernel and bios(ΦωΦ)
    init_jmptab();

    // Init task information (〃'▽'〃)
    init_task_info();

    // Output 'Hello OS!', bss check result and OS version
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int i, output_val_pos = 0;

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    for (i = 0; i < sizeof(output_str); ++i)
    {
        buf[i] = output_str[i];
        if (buf[i] == '_')
        {
            buf[i] = output_val[output_val_pos++];
        }
    }

    bios_putstr("Hello OS!\n\r");
    bios_putstr(buf);
    clearbuf();

    /*task[2] 读取终端并回显*/
    // int ch;
    // while(1){
    //     while((ch = bios_getchar()) == -1)
    //         ;
    //     if(ch == '\r'){
    //         bios_putstr("\n\r");
    //     }else{
    //         bios_putchar(ch);
    //     }
    // }

    // TODO: Load tasks by either task id [p1-task3] or task name [p1-task4],
    //   and then execute them.
    /*task3*/
    // int task_id;
    // uint64_t entrypoint;
    // void (*entryfunc)(void);
    // while(1){
    //     while((task_id = bios_getchar()) == -1)
    //         ;
    //     bios_putchar(task_id);
    //     task_id = task_id - '0';
    //     if(task_id >= 0 && task_id <= TASK_MAXNUM){
    //         bios_putchar('\n');
    //         entrypoint = load_task_img(task_id);
    //         ((void(*)())entrypoint)();
    //         //entryfunc = (void *)entrypoint;
    //         //entryfunc();
    //     }
    //     else{
    //         bios_putstr("Invalid task id!\n\r");
    //     }
    // }

    /*task4*/
    int ch;
    int index = 0;// 缓冲区中索引
    uint64_t entrypoint;
    int id = 0;
    while(1){
        while((ch = bios_getchar()) == -1)
            ;
        // que: 为什么无法实现退格
        // if(ch == 8){
        //     index = index ? index - 1 : index;
        //     bios_putchar('\127');
        //     continue;
        // }
        if(index >= VERSION_BUF - 1){
            bios_putstr("\n\rWARNING: the buf is full\n\r");
			index = 0;
            clearbuf();
            continue;// 缓冲区满时清空重新读入
        }
        if(ch != '\r'){
            buf[index++] = ch;
            bios_putchar(ch);
        }
        //task5
        else if(!strcmp(buf,"batch")){
            buf[index] = '\0';
            index = 0;
            bios_putchar('\n');
            local_flush_icache_all();

            for(id = 0; id < tasknum; id++){
                if(!strcmp(tasks[id].name, "batchprint")){
                    entrypoint = load_task_img(id);
                    ((void(*)())entrypoint)();
                    break;
                }
            }
            local_flush_icache_all();
            for(id = 0; id < tasknum; id++){
                if(!strcmp(tasks[id].name, "batchsort")){
                    entrypoint = load_task_img(id);
                    ((void(*)())entrypoint)();
                    break;
                }
            }
            local_flush_icache_all();
            for(id = 0; id < tasknum; id++){
                if(!strcmp(tasks[id].name, "batchdedup")){
                    entrypoint = load_task_img(id);
                    ((void(*)())entrypoint)();
                    break;
                }
            }
            local_flush_icache_all();
            int ch;
            // for(ch = 0; ch < BATCH_STR_NUM && *(char *)(BATCH_STR_STA + BATCH_STR_SPA * ch); ch++){
            //     bios_putstr("line:[");
            //     bios_putchar('0'+ch);
            //     bios_putstr("]:");
            //     bios_putstr(BATCH_STR_STA + BATCH_STR_SPA * ch);
                
            // }
            // bios_putstr("line:[");
            // bios_putchar('0'+ch);
            // bios_putstr("]:");
            // bios_putstr("Guo ZiYing\n\r");
            for(id = 0; id < tasknum; id++){
                if(!strcmp(tasks[id].name, "batchout")){
                    entrypoint = load_task_img(id);
                    ((void(*)())entrypoint)();
                    break;
                }
            }
            clearbuf();
        }
        else{
            buf[index] = '\0';
            index = 0;
            bios_putchar('\n');
            local_flush_icache_all();
            for(id = 0; id < tasknum; id++){
                if(!strcmp(tasks[id].name, buf)){
                    entrypoint = load_task_img(id);
                    ((void(*)())entrypoint)();
                    break;
                }
            }
            if(id == tasknum){
                bios_putstr("WARNING: No such task!\n\r");
            }
            clearbuf();
        }
    }




    

    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        asm volatile("wfi");
    }

    return 0;
}
