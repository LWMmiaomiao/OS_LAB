#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <os/string.h>
#include <atomic.h>
#include <printk.h>

mutex_lock_t mlocks[LOCK_NUM];
barrier_t barriers[BARRIER_NUM];
condition_t conditions[CONDITION_NUM];
mailbox_t mailboxes[MBOX_NUM];

void init_locks(void)
{
	/* TODO: [p2-task2] initialize mlocks */
	for(int i = 0; i < LOCK_NUM; i++)
	{
		mlocks[i].block_queue.next = mlocks[i].block_queue.prev = &mlocks[i].block_queue;
		mlocks[i].lock.status = UNLOCKED;
		mlocks[i].key = -1;
        mlocks[i].pid = -1;
		mlocks[i].usage = UNUSED;
	}
}

void spin_lock_init(spin_lock_t *lock)
{
	/* TODO: [p2-task2] initialize spin lock */
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
	/* TODO: [p2-task2] try to acquire spin lock */
	return 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
	/* TODO: [p2-task2] acquire spin lock */
}

void spin_lock_release(spin_lock_t *lock)
{
	/* TODO: [p2-task2] release spin lock */
}

int do_mutex_lock_init(int key)
{
	/* TODO: [p2-task2] initialize mutex lock */
    for(int i = 0;i < LOCK_NUM; i++){
        if(mlocks[i].usage == USING && mlocks[i].key == key)
            return i;
    }
    // 寻找空闲锁
    for(int i = 0; i < LOCK_NUM; i++){
        if(mlocks[i].usage == UNUSED){
            mlocks[i].usage = USING;
            mlocks[i].key = key;
            return i;
        }
    }
    return -1;  // 无空闲锁
}

void do_mutex_lock_acquire(int mlock_idx)
{
	/* TODO: [p2-task2] acquire mutex lock */

	/* NOTE: This is non-reentrant mutex.
	   Multiple requests for a lock from the same process can lead to deadlocks */
	// The interrupt is disabled gloablly in S-mode, but still using atomic operation
	// The process tries to acquire the lock until it succeed
	// For details, see README.md
	while(atomic_swap(LOCKED, (ptr_t)&mlocks[mlock_idx].lock.status) == LOCKED)
		do_block(&current_running->list, &mlocks[mlock_idx].block_queue);
	mlocks[mlock_idx].lock.status = LOCKED;
	mlocks[mlock_idx].pid = current_running->pid;
	//current_running->mlock_idx = mlock_idx;
}

void do_mutex_lock_release(int mlock_idx)
{
	/* TODO: [p2-task2] release mutex lock */

	mlocks[mlock_idx].lock.status = UNLOCKED;
	mlocks[mlock_idx].pid = -1;

	if(&mlocks[mlock_idx].block_queue != mlocks[mlock_idx].block_queue.next)	// queue is not empty
		do_unblock(mlocks[mlock_idx].block_queue.next);
	//current_running->mlock_idx = -1;
}

void do_mutex_lock_release_bypid(pid_t pid)
{
	for(int i = 0; i < LOCK_NUM; i++){
        if(mlocks[i].pid == pid && mlocks[i].usage == USING)
            do_mutex_lock_release(i);
    }
}

/*--------------------------------------------Barrier------------------------------------*/

void init_barriers(void)
{
	for(int i = 0; i < BARRIER_NUM; i++)
	{
		barriers[i].block_queue.next = barriers[i].block_queue.prev = &barriers[i].block_queue;
		barriers[i].block_queue.pcb_ptr = (ptr_t)NULL;
		barriers[i].goal = barriers[i].arrived = 0;
		barriers[i].usage = UNUSED;
		barriers[i].key = -1;// key为-1可以表示UNUSED, 后续可修改删去usage
	}
}

int do_barrier_init(int key,int goal)
{
    // 寻找对应key是否已经有对应屏障变量
    for(int i = 0; i < BARRIER_NUM; i++){
        if(barriers[i].usage == USING && barriers[i].key == key){ // 找到匹配屏障变量
            barriers[i].goal = goal;// 不应修改arrived
            return i;
        }
    }
    // 寻找空闲屏障变量
    for(int i = 0; i < BARRIER_NUM; i++){
        if(barriers[i].usage == UNUSED){ // 找到空闲屏障变量
            barriers[i].key = key;
            barriers[i].goal = goal;
			barriers[i].arrived = 0;
            return i;
        }
    }
	printl("WARNING:NO MORE AVAILABLE BARRIERS! key: %d\n", key);
    return -1;  // 未找到, 向日志报错返回-1
}

void do_barrier_wait(int bar_idx)
{
	barriers[bar_idx].arrived++;
	if(barriers[bar_idx].arrived < barriers[bar_idx].goal)
		do_block(&current_running->list, &barriers[bar_idx].block_queue);
	else
	{
		freeQueueToReady(&barriers[bar_idx].block_queue);
		barriers[bar_idx].arrived = 0;
	}
}

void do_barrier_destroy(int bar_idx)
{
	freeQueueToReady(&barriers[bar_idx].block_queue);
	barriers[bar_idx].goal = 0;
	barriers[bar_idx].arrived = 0;
	barriers[bar_idx].key = -1;
	barriers[bar_idx].usage = UNUSED;
}

/*--------------------------------------------Condition------------------------------------*/

void init_conditions(void)
{
	for(int i = 0; i < CONDITION_NUM; i++)
	{
		conditions[i].block_queue.next = conditions[i].block_queue.prev = &conditions[i].block_queue;
		conditions[i].key = -1;// key为-1可以表示UNUSED, 后续可修改删去usage
        conditions[i].usage = UNUSED;
	}
}

int do_condition_init(int key)
{
    // 寻找对应key是否已经有对应条件变量
    for(int i=0; i < CONDITION_NUM; i++){
        if(conditions[i].usage == USING && conditions[i].key == key){ // 找到匹配条件变量
            return i;
        }
    }
    // 寻找空闲屏障变量
    for(int i = 0; i < CONDITION_NUM; i++){
        if(conditions[i].usage == UNUSED){ // 找到空闲条件变量
            conditions[i].key = key;
            return i;
        }
    }
	printl("WARNING:NO MORE AVAILABLE CONDITIONS! key: %d\n", key);
    return -1;  // 未找到, 向日志报错返回-1
}

/* 将线程放入等待队列，并释放当前持有的互斥锁，
 * 使其他线程可以修改条件变量。一直阻塞，直到另一个线程唤醒它为止。
 * 当被唤醒时，它会重新获取互斥锁。*/
void do_condition_wait(int cond_idx, int mutex_idx)
{
	do_mutex_lock_release(mutex_idx);
	do_block(&current_running->list, &conditions[cond_idx].block_queue);
	do_mutex_lock_acquire(mutex_idx);
}

void do_condition_signal(int cond_idx)
{
	if(conditions[cond_idx].block_queue.next != &conditions[cond_idx].block_queue)
	{
		do_unblock(conditions[cond_idx].block_queue.next);
	}
	else{
		printl("WARNING:NO AVAILABLE CONDITION TO SIGNAL! cond_idx: %d\n", cond_idx);
	}
}

void do_condition_broadcast(int cond_idx)
{
	if(conditions[cond_idx].block_queue.next != &conditions[cond_idx].block_queue)
	{
		freeQueueToReady(&conditions[cond_idx].block_queue);
	}
	else{
		printl("WARNING:NO AVAILABLE CONDITION TO BROADCAST! cond_idx: %d\n", cond_idx);
	}
		
}

void do_condition_destroy(int cond_idx)
{
	do_condition_broadcast(cond_idx);
	conditions[cond_idx].key = -1;
	conditions[cond_idx].usage = UNUSED;
}

/*--------------------------------------------Mailbox Interface------------------------------------*/
void init_mbox()
{
	for(int i = 0; i < MBOX_NUM; i++)
	{
		mailboxes[i].name[0] = '\0';
		mailboxes[i].rev_block_queue.next = mailboxes[i].rev_block_queue.prev = &mailboxes[i].rev_block_queue;
		mailboxes[i].send_block_queue.next = mailboxes[i].send_block_queue.prev = &mailboxes[i].send_block_queue;
		mailboxes[i].remain_length = MAX_MBOX_LENGTH;
		mailboxes[i].wcur = mailboxes[i].rcur = 0;
		mailboxes[i].ref_cnt = 0;
	}
}

int do_mbox_open(char *name)
{
	for(int i = 0; i < MBOX_NUM; i++) // 存在对应mbox
	{
		if(mailboxes[i].name[0] != '\0' && strcmp(mailboxes[i].name, name) == 0)
		{
			mailboxes[i].ref_cnt++;
			current_running->mbox_idx = i;
			return i;
		}
	}
	for(int i = 0; i < MBOX_NUM; i++) // 存在空闲mbox
	{
		if(mailboxes[i].name[0] == '\0')	//ref_cnt == 0
		{
			strcpy(mailboxes[i].name, name);
			mailboxes[i].ref_cnt++;
			current_running->mbox_idx = i;
			return i;
		}
	}
	printl("WARNING:NO MORE AVAILABLE MAILBOX!\n");
	return -1;
}

void do_mbox_close(int mbox_idx)
{
	mailboxes[mbox_idx].ref_cnt--;
	if(mailboxes[mbox_idx].ref_cnt == 0) // 转为空闲mbox
	{
		mailboxes[mbox_idx].name[0] = '\0';
		mailboxes[mbox_idx].wcur = mailboxes[mbox_idx].rcur = 0;
		mailboxes[mbox_idx].remain_length = MAX_MBOX_LENGTH;
		current_running->mbox_idx = -1;
	}
	else if(mailboxes[mbox_idx].ref_cnt < 0)
	{
		printl("ERROR: In function do_mbox_close(), ref_cnt < 0");
	}
}

int do_mbox_send(int mbox_idx, void * msg, int msg_length)
{
	int blocked = 0;
	while(1)
	{
		if(msg_length > mailboxes[mbox_idx].remain_length)
		{
			do_block(&current_running->list, &mailboxes[mbox_idx].send_block_queue);
			blocked += 1;
		}
		else
		{
			for(int i = 0; i < msg_length; i++)
			{
				mailboxes[mbox_idx].msg_array[mailboxes[mbox_idx].wcur] = ((uint8_t *)msg)[i];
				mailboxes[mbox_idx].wcur = (mailboxes[mbox_idx].wcur + 1) % MAX_MBOX_LENGTH;
			}
			mailboxes[mbox_idx].remain_length -= msg_length;
			if(mailboxes[mbox_idx].rev_block_queue.next != &mailboxes[mbox_idx].rev_block_queue)
				freeQueueToReady(&mailboxes[mbox_idx].rev_block_queue);
			return blocked;
		}
	}
}

int do_mbox_recv(int mbox_idx, void *msg, int msg_length)
{
	int blocked = 0;
	while(1)
	{
		if(msg_length > MAX_MBOX_LENGTH - mailboxes[mbox_idx].remain_length)
		{
			do_block(&current_running->list, &mailboxes[mbox_idx].rev_block_queue);
			blocked += 1;
		}
		else
		{
			for(int i = 0; i < msg_length; i++)
			{
				((uint8_t *)msg)[i] = mailboxes[mbox_idx].msg_array[mailboxes[mbox_idx].rcur];
				mailboxes[mbox_idx].rcur = (mailboxes[mbox_idx].rcur + 1) % MAX_MBOX_LENGTH;
			}
			mailboxes[mbox_idx].remain_length += msg_length;
			if(mailboxes[mbox_idx].send_block_queue.next != &mailboxes[mbox_idx].send_block_queue)
				freeQueueToReady(&mailboxes[mbox_idx].send_block_queue);
			return blocked;
		}
	}
}