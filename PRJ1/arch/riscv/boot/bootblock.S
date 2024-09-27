#include <asm/biosdef.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc
// taskinfo block number location 
.equ taskinfo_block_num, 0x502001fa
// taskinfo begin block location 
.equ taskinfo_block_begin, 0x502001f8


// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50201000
//taskinfo address (add in p1-t4)
.equ taskinfo, 0x50200200

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// TODO: [p1-task1] call BIOS to print string "It's bootblock!"
	li a7, BIOS_PUTSTR
	la a0, msg
	jal bios_func_entry

	// TODO: [p1-task2] call BIOS to read kernel in SD card
	li a7, BIOS_SDREAD
	li a0, 0x50201000
	li t0, 0x502001fc
	lh a1, 0(t0)
	li a2, 1
	jal bios_func_entry

	// TODO: [p1-task4] load task-related arguments and pass them to kernel

	la a0, 0x52000000 
	li t0, taskinfo_block_num
	lh a1, 0(t0)
	li t0, taskinfo_block_begin
	lh a2, 0(t0)
	li a7, BIOS_SDREAD
	jal bios_func_entry

	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	jal 0x50201000

	/************************************************************/
	/* Do not touch this comment. Reserved for future projects. */
	/************************************************************/
// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's Guo ZiYing's bootloader...\n\r"

