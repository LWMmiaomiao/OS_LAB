#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>

mutex_lock_t mlocks[LOCK_NUM];

void init_locks(void)
{
    /* TODO: [p2-task2] initialize mlocks */
    for(int i = 0; i < LOCK_NUM; i++){
        mlocks[i].key = -1;
        mlocks[i].block_queue.prev = mlocks[i].block_queue.next = &mlocks[i].block_queue;// 设为NULL的话addQueue需要增加其他判定
        mlocks[i].lock.status = UNLOCKED;
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
    for(int i = 0; i < LOCK_NUM; i++){
        if(mlocks[i].key == key){
            return i;
        }
    }
    for(int i = 0; i <LOCK_NUM; i++){
        if(mlocks[i].key == -1){
            mlocks[i].key = key;
            return i;
        }
    }
    return -1;
}

void do_mutex_lock_acquire(int mlock_idx)
{
    /* TODO: [p2-task2] acquire mutex lock */
    while(atomic_cmpxchg(UNLOCKED, LOCKED, (ptr_t)(&mlocks[mlock_idx].lock.status)) == LOCKED){
        do_block(&current_running->list, &mlocks[mlock_idx].block_queue);
    }
    // while (test-and-set(value)) {
    //     add this TCB to wait queue q; 
    //     schedule();
    // }
}

void do_mutex_lock_release(int mlock_idx)
{
    /* TODO: [p2-task2] release mutex lock */
    list_node_t *release_node = returnTop(&mlocks[mlock_idx].block_queue);
    mlocks[mlock_idx].lock.status = UNLOCKED;
    do_unblock(release_node);
}
