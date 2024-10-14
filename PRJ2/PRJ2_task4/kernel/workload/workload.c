#include <os/time.h>
#include <os/sched.h>
#include <os/workload.h>
#include <printk.h>

fly_info fly[FLY_NUM];
int sort_ok = 5;
int sort_pid[5] = {0, 1, 2, 3, 4};// sort_order to pid
void initRemainSort()
{
    int temp;
    int j;
    for(int i = 1;i < 5; i++){
        if(fly[sort_pid[i]].init_remain < fly[sort_pid[i-1]].init_remain){
            temp = sort_pid[i];
            for(j = i-1;j >= 0 && fly[temp].init_remain < fly[sort_pid[j]].init_remain; j--){
                sort_pid[j+1] = sort_pid[j];
            }
            sort_pid[j+1] = temp;
        }
    }
}
bool ASlowerB(int a, int b){// 参数为pid a, pid b返回a是否慢于b
	if(fly[a].reach < fly[b].reach){
		return 1;
	}
	else if(fly[a].reach > fly[b].reach){
		return 0;
	}
	else{
		return fly[a].remain_length > fly[b].remain_length;
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
	if(fly[pid].process_id == 0)
	{
			fly[pid].remain_length = fly;
			fly[pid].init_remain = remain + 1;
			fly[pid].time_interval = TIMER_INTERVAL / 3;
			fly[pid].process_id = i + 8;
			fly[pid].prev_time = 0;
			fly[pid].reach = 0;
			sort_ok--;
			printl("success init pid %d!\n", pid);
			do_scheduler();
	}
	if(sort_ok == 0){
		sort_ok--;
		initRemainSort();
		printl("success init sort!\n");
		for(int i = 0; i < 5; i++){
			fly[sort_pid[i]].init_order = i;
			printl("init: pid %d order %d\n", sort_pid[i], i);
		}
	}
	if(fly[pid].process_id != 0 && remain > fly[pid].remain_length)// 如果是remain == 0会出问题
	{
		status = 0;
		fly[pid].reach += 1;
		printl("pid: %d, reach: %d\n",
		current_running->pid,fly[pid].reach);
	}
	fly[pid].remain_length = remain;

	// 每次调用，基于相关的数据计算本次和上次调用之间经过的时间
	interval = get_ticks() - current_running->start_tick;
	s = interval;

	// 根据飞机的飞行轮次和调用间隔分配时间片
	for(i=0;i<FLY_NUM;i++)
	{
		if(fly[i].reach < min)
			min = fly[i].reach;
	}
	for(i=0;i<FLY_NUM;i++)
	{
		if(fly[i].reach > max)
			max = fly[i].reach;
	}

	if(fly[pid].reach == min)
	{
		current_running->time_chunk = ((remain + 4 * fly[pid].init_order) *  TIMER_INTERVAL)  + 1000;
		if(fly[pid].reach != max){// 对于轮数落后的分配更多时间片
			current_running->time_chunk = ((remain + 4 * fly[pid].init_order) * 5 * TIMER_INTERVAL)  + 5000;
		}
		if(current_running->time_chunk < s)		// 分配的时间片小于当前的运行时间，立刻切换进程
			do_scheduler();
	}
	else
	{
		current_running->time_chunk = (remain +  4 * fly[pid].init_order *  TIMER_INTERVAL) / SPEEDING_PENALTY + 500;
		do_scheduler();			// 飞行轮次不等于最小值，立刻切换进程
	}

	// 防止超车设计
	if(fly[pid].init_order != 0){
		int pre_sort = fly[pid].init_order - 1;
		int pre_pid = sort_pid[pre_sort];
		if(ASlowerB(pre_pid, pid)){
			
			do_scheduler();
		}
	}
}