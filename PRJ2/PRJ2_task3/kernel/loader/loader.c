#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <type.h>

uint64_t load_task_img(int taskid)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */
    // taskid从0开始计数
    //task3
    //bios_sd_read((TASK_MEM_BASE + taskid * TASK_SIZE),tasks[taskid].block_num,tasks[taskid].block_id);
	//return (TASK_MEM_BASE + taskid * TASK_SIZE);
    //uint64_t entrypoint = TASK_MEM_BASE + TASK_SIZE * taskid;
    //bios_sd_read(0x52000000+0x10000*taskid,15,1+(15*(taskid+1)));
    //return 0x52000000+0x10000*taskid;
    //bios_sd_read(entrypoint, 15, 1 + 15 * (taskid+1));
    //return entrypoint;

    uint16_t block_num = NBYTES2SEC(tasks[taskid].begin % SECTOR_SIZE + tasks[taskid].size);// 当前task所占扇区数目
    uint16_t block_begin = tasks[taskid].begin / SECTOR_SIZE;// 当前task起始扇区
    uint64_t entrypoint = tasks[taskid].entrypoint;
    // 先调用API将数据从creatimage搬运到内存, 再在内存中调用库函数调整正确位置清空padding
    bios_sd_read(entrypoint, block_num, block_begin);
    memcpy((uint8_t *)entrypoint,(uint8_t *)(entrypoint + tasks[taskid].begin % SECTOR_SIZE),tasks[taskid].size);
    return entrypoint;
}

uint64_t load_task_img_name(char *taskname){
    int id;
    // compare task name one by one
    for(id = 0; id < tasknum; id++){
        if(strcmp(tasks[id].name, taskname)==0){
            return load_task_img(id);
        }
    }
    if(id == tasknum){
        if(*taskname=='\0'){
            bios_putstr("WARNING:Task name empty!\n\r");
        }else{
            bios_putstr("WARNING:No task named ");
            bios_putstr(taskname);
            bios_putstr("!\n\r");
        }
    }
    return 0;
}