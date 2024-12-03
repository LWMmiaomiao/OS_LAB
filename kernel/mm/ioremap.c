#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>
#include <os/page.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // TODO: [p5-task1] map one specific physical region to virtual address
	printl("entry ioremap!%ld %ld\n", phys_addr, size);
    uint64_t page_num =
		(size / PAGE_SIZE) + ((size % PAGE_SIZE != 0) ? 1 : 0);
	void *ret = (void *)io_base;
	for (uint64_t i = 0; i < page_num; ++i) {
		map_page(io_base, phys_addr, (PTE *)pa2kva(PGDIR_PA), 0);
		io_base += PAGE_SIZE;
		phys_addr += PAGE_SIZE;
	}
	local_flush_tlb_all();
	return ret;
}

void iounmap(void *io_addr)
{
    // TODO: [p5-task1] a very naive iounmap() is OK
    // maybe no one would call this function?
}
