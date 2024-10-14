#include <os/time.h>
#include <os/sched.h>
#include <os/workload.h>
#include <printk.h>

fly_len_t length[FLY_NUM];
int sort_ok = 5;
int sort_pid[5] = {0, 1, 2, 3, 4};// sort_order to pid
void initRemainSort()
{
    int temp;
    int j;
    for(int i = 1;i < 5; i++){
        if(length[sort_pid[i]].init_remain < length[sort_pid[i-1]].init_remain){
            temp = sort_pid[i];
            for(j = i-1;j >= 0 && length[temp].init_remain < length[sort_pid[j]].init_remain; j--){
                sort_pid[j+1] = sort_pid[j];
            }
            sort_pid[j+1] = temp;
        }
    }
}
bool ASlowerB(int a, int b){// 参数为pid a, pid b返回a是否慢于b
	if(length[a].reach < length[b].reach){
		return 1;
	}
	else if(length[a].reach > length[b].reach){
		return 0;
	}
	else{
		return length[a].remain_length > length[b].remain_length;
	}
}
void do_workload_schedule(uint64_t remain)
{
	int i = 0;
	int status = 1;
	int min = INT32_MAX;
	int max = 0;
	int pid = process_id - 8;
	uint64_t interval = 0, s = 0;
	uint64_t total;
	if(length[pid].process_id == 0)
	{
			length[pid].remain_length = LENGTH;
			length[pid].init_remain = remain + 1;
			length[pid].time_interval = TIMER_INTERVAL / 3;
			length[pid].process_id = i + 8;
			length[pid].prev_time = 0;
			length[pid].reach = 0;
			sort_ok--;
			printl("success init pid %d!\n", pid);
			do_scheduler();
	}
	if(sort_ok == 0){
		sort_ok--;
		initRemainSort();
		printl("success init sort!\n");
		for(int i = 0; i < 5; i++){
			length[sort_pid[i]].init_order = i;
			printl("init: pid %d order %d\n", sort_pid[i], i);
		}
	}
	if(length[pid].process_id != 0 && remain > length[pid].remain_length)// 如果是remain == 0会出问题
	{
		status = 0;
		length[pid].reach += 1;
		printl("pid: %d, reach: %d\n",
		current_running->pid,length[pid].reach);
	}
	length[pid].remain_length = remain;

	// 每次调用，基于相关的数据计算本次和上次调用之间经过的时间
	interval = get_ticks() - current_running->start_tick;
	s = interval;

	// 根据飞机的飞行轮次和调用间隔分配时间片
	for(i=0;i<FLY_NUM;i++)
	{
		if(length[i].reach < min)
			min = length[i].reach;
	}
	for(i=0;i<FLY_NUM;i++)
	{
		if(length[i].reach > max)
			max = length[i].reach;
	}

	if(length[pid].reach == min)
	{
		current_running->time_chunk = ((remain + 4 * length[pid].init_order) *  TIMER_INTERVAL)  + 1000;
		if(length[pid].reach != max){// 对于轮数落后的分配更多时间片
			current_running->time_chunk = ((remain + 4 * length[pid].init_order) * 5 * TIMER_INTERVAL)  + 5000;
		}
		if(current_running->time_chunk < s)		// 分配的时间片小于当前的运行时间，立刻切换进程
			do_scheduler();
	}
	else
	{
		current_running->time_chunk = (remain +  4 * length[pid].init_order *  TIMER_INTERVAL) / SPEEDING_PENALTY + 500;
		do_scheduler();			// 飞行轮次不等于最小值，立刻切换进程
	}

	// 防止超车设计
	if(length[pid].init_order != 0){
		int pre_sort = length[pid].init_order - 1;
		int pre_pid = sort_pid[pre_sort];
		if(ASlowerB(pre_pid, pid)){
			
			do_scheduler();
		}
	}
}