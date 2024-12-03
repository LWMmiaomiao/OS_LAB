#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/page.h>
#include <assert.h>

// NOTE: A/C-core
static ptr_t kernMemCurr = FREEMEM_KERNEL;

ptr_t allocPage(int numPage)
{
    // align PAGE_SIZE
    ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
    kernMemCurr = ret + numPage * PAGE_SIZE;
    return ret;
}

// NOTE: Only need for S-core to alloc 2MB large page
#ifdef S_CORE
static ptr_t largePageMemCurr = LARGE_PAGE_FREEMEM;
ptr_t allocLargePage(int numPage)
{
    // align LARGE_PAGE_SIZE
    ptr_t ret = ROUND(largePageMemCurr, LARGE_PAGE_SIZE);
    largePageMemCurr = ret + numPage * LARGE_PAGE_SIZE;
    return ret;    
}
#endif

// void freePage(ptr_t baseAddr)
// {
//     // TODO [P4-task1] (design you 'freePage' here if you need):
// }

void *kmalloc(size_t size)
{
    // TODO [P4-task1] (design you 'kmalloc' here if you need):
}


/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO [P4-task1] share_pgtable:
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO [P4-task1] alloc_page_helper:
}

uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
	int idx = -1;
	for (int i = 0; i < SHARE_PAGE_NUMS; ++i) {
		if (key == sharepgcb[i].key) {
			idx = i;
			break;
		}
	}
	if (idx == -1) {
		for (int i = 0; i < SHARE_PAGE_NUMS; ++i) {
			if (key == sharepgcb[i].key || sharepgcb[i].key == -1) {
				sharepgcb[i].key = key;
				idx = i;
				break;
			}
		}
	}
	++sharepgcb[idx].user_num;
	if (sharepgcb[idx].user_num == 1) {
		sharepgcb[idx].addr = allocPage_pin(0, 0, PINNED);
	}
	uint64_t va = -1;
	for (uint64_t i = PAGE_SIZE; i < 0x0000003ffffffffflu; i += PAGE_SIZE) {
		if (!valid_va(i, current_running->pagedir)) {
			va = i;
			break;
		}
	}
	map_page(va, kva2pa(sharepgcb[idx].addr), current_running->pagedir,
		 current_running->pid);
	return va;
}

void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
	ptr_t kvaddr =
		pa2kva(get_pa(*getEntry(current_running->pagedir, addr)));
	*getEntry(current_running->pagedir, addr) = 0;
	int idx = -1;
	for (int i = 0; i < SHARE_PAGE_NUMS; ++i) {
		if (kvaddr == sharepgcb[i].addr) {
			idx = i;
			break;
		}
	}
	--sharepgcb[idx].user_num;
	if (sharepgcb[idx].user_num == 0) {
		freePage_pgcb(&pgcb[addr2idx(sharepgcb[idx].addr)]);
		sharepgcb[idx].addr = 0;
		sharepgcb[idx].key = -1;
	}
}

// add p4 task1
ptr_t allocPage_pin(int pid, uint64_t vaddr, pg_pin_status_t pin)
{
	pgcb_t *pg = NULL;
	for (int i = 0; i < PAGE_NUMS; ++i) {
		if (pgcb[i].status == FREE) {
			pg = &pgcb[i];
			break;
		}
	}
	if (pg == NULL) {
		printl("alloc to swap, pid %d vaddr %d\n", pid, vaddr);
		pg = swapOut();
		printl("return swapout pg\n");
	}
	pg->status = ALLOC;
	pg->pid = pid;
	pg->vaddr = vaddr;
	pg->pin = pin;
	clear_pgdir(pg->addr);
	return pg->addr;
}

void freePage_pgcb(pgcb_t *pg)
{
	// TODO [P4-task1] (design you 'freePage' here if you need):
	clear_pgdir(pg->addr);
	pg->status = FREE;
	pg->pin = UNPINNED;
	pg->pid = 0;
	pg->vaddr = 0;
}
int addr2idx(ptr_t addr)
{
	return (addr - FREEMEM_KERNEL) / PAGE_SIZE;
}

int idx2sectorIdx(int idx)
{
	return idx * 8;
}

void initkmem()
{
	for (int i = 0; i < SHARE_PAGE_NUMS; ++i) {
		sharepgcb[i].key = -1;
		sharepgcb[i].user_num = 0;
		sharepgcb[i].addr = 0;
	}
	for (int i = 0; i < MEM_PAGE_NUMS; ++i) {
		mempgcb[i].pid = 0;
		mempgcb[i].status = FREE;
		mempgcb[i].vaddr = 0;
	}
	for (int i = 0; i < PAGE_NUMS; ++i) {
		ptr_t addr = i * PAGE_SIZE + FREEMEM_KERNEL;
		pgcb[i].addr = addr;
		pgcb[i].status = FREE;
		pgcb[i].pin = UNPINNED;
		pgcb[i].pid = 0;
		pgcb[i].vaddr = 0;
	}
}