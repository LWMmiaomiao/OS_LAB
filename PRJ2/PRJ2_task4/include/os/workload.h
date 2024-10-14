#ifndef INCLUDE_WORKLOAD_H
#define INCLUDE_WORKLOAD_H

#include <os/sched.h>

#define LENGTH 60
#define FLY_NUM 5		// 飞机数为5
#define SPEEDING_PENALTY 20	// 当有飞机飞行次数比其他的都大时，进行惩罚，缩短分配的时间片

typedef struct fly_info
{
	uint64_t prev_time;		// 上一次调用sys_set_sche_workload时，该进程运行的时间
	uint64_t time_interval;		// 本次和上次调用之间，该进程运行了多久
	int reach;// 飞机飞行轮次
	int remain_length;		// 飞机距离终点的位置
	int process_id;
	int pid;			// 对应进程的pid
	int init_remain;
	int init_order;
} fly_info;

// 结构体数组
extern fly_info fly[FLY_NUM];

extern void do_workload_schedule(uint64_t remain);

#endif