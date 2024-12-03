#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/loader.h>
#include <type.h>


// uint64_t load_task_img(int taskid)
// {
// 	/**
// 	* TODO:
// 	* 1. [p1-task3] load task from image via task id, and return its entrypoint
// 	* 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
// 	*/
// 	// APP X starts from (0x52000000 + X * 0x10000)
// 	bios_sd_read((TASK_MEM_BASE + taskid * TASK_SIZE),tasks[taskid].block_num,tasks[taskid].block_id);
// 	return (TASK_MEM_BASE + taskid * TASK_SIZE);
// }

uint64_t load_task_img_by_name(char * str)
{
	for(int i = 0; i < TASK_MAXNUM; i++)
	{
		if(strcmp(tasks[i].filename,str)==0)
		{
			bios_sd_read((TASK_MEM_BASE + i * TASK_SIZE), tasks[i].block_num, tasks[i].block_id);
			return (TASK_MEM_BASE + i * TASK_SIZE);
		}
	}
	return 0;
}

// uint64_t from_name_load_task_img(char *name, pcb_t *pcb)
// {
// 	int idx = -1;
// 	for (int taskidx = 0; taskidx < TASK_MAXNUM; ++taskidx) {
// 		if (strcmp(tasks[taskidx].filename, name) == 0) {
// 			idx = taskidx;
// 		}
// 	}
// 	char buf[SECTOR_SIZE];
// 	if (idx == -1)
// 		return 1;
// 	uint64_t kvpa = allocPage_pin(pcb->pid, 0, PINNED);
// 	map_page((TASK_MEM_BASE + idx * TASK_SIZE), kva2pa(kvpa), pcb->pagedir, pcb->pid);
// 	uint32_t task_sector_id = tasks[idx].block_id;
// 	bios_sd_read((unsigned int)kva2pa((uintptr_t)buf), 1, task_sector_id++);
// 	for (int i = 0; i < tasks[idx].block_num * SECTOR_SIZE; ++i) {
// 		if (i && (i + tasks[idx].offset) % SECTOR_SIZE == 0) {
// 			bios_sd_read((unsigned int)kva2pa((uintptr_t)buf), 1,
// 				     task_sector_id++);
// 		}
// 		if (i && i % PAGE_SIZE == 0) {
// 			kvpa = allocPage(pcb->pid, 0, PINNED);
// 			map_page(tasks[idx].entrypoint + i, kva2pa(kvpa),
// 				 pcb->pagedir, pcb->pid);
// 		}
// 		((char *)kvpa)[i % PAGE_SIZE] =
// 			buf[(i + tasks[idx].offset) % SECTOR_SIZE];
// 	}
// }

uint64_t load_task_img(int taskid)
{
	/**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */
	// [p1-task3]
	// bios_sd_read(TASK_MEM_BASE + TASK_SIZE * taskid, 15, 1 + 15 * (taskid + 1));
	// return TASK_MEM_BASE + TASK_SIZE * taskid;

	// [p1-task4]
	uint32_t task_sector_id = tasks[taskid].offset / SECTOR_SIZE;
	uint32_t task_sector_num =
		(tasks[taskid].offset + tasks[taskid].size) / SECTOR_SIZE -
		task_sector_id + 1;
	bios_sd_read(tasks[taskid].entrypoint, task_sector_num, task_sector_id);
	memcpy((uint8_t *)(tasks[taskid].entrypoint),
	       (uint8_t *)(tasks[taskid].entrypoint +
			   (tasks[taskid].offset % SECTOR_SIZE)),
	       tasks[taskid].size);
	return tasks[taskid].entrypoint;
}

uint64_t getEntrypoint(char *name)
{
	for (int taskidx = 0; taskidx < TASK_MAXNUM; ++taskidx) {
		if (strcmp(tasks[taskidx].filename, name) == 0) {
			return tasks[taskidx].entrypoint;
		}
	}
	return 0;
}

uint64_t from_name_load_task_img(char *name, pcb_t *pcb)
{
	int idx = -1;
	for (int taskidx = 0; taskidx < TASK_MAXNUM; ++taskidx) {
		if (strcmp(tasks[taskidx].filename, name) == 0) {
			idx = taskidx;
		}
	}
	char buf[SECTOR_SIZE];
	if (idx == -1)
		return 1;
	uint64_t kvpa = allocPage_pin(pcb->pid, 0, PINNED);
	map_page(tasks[idx].entrypoint, kva2pa(kvpa), pcb->pagedir, pcb->pid);
	uint32_t task_sector_id = tasks[idx].offset / SECTOR_SIZE;
	bios_sd_read((unsigned int)kva2pa((uintptr_t)buf), 1, task_sector_id++);
	for (int i = 0; i < tasks[idx].size; ++i) {
		if (i && (i + tasks[idx].offset) % SECTOR_SIZE == 0) {
			bios_sd_read((unsigned int)kva2pa((uintptr_t)buf), 1,
				     task_sector_id++);
		}
		if (i && i % PAGE_SIZE == 0) {
			kvpa = allocPage_pin(pcb->pid, 0, PINNED);
			map_page(tasks[idx].entrypoint + i, kva2pa(kvpa),
				 pcb->pagedir, pcb->pid);
		}
		((char *)kvpa)[i % PAGE_SIZE] =
			buf[(i + tasks[idx].offset) % SECTOR_SIZE];
	}
}
