#include "regs.h"
	.text
	.align	2
	.globl  get_timer	
	.ent	get_timer
	.type	get_timer, @function
get_timer:
	li   $9,0x9
	mfc0 $2,$9
        nop
        j       ra
        nop
	.align	2
	.globl  kernel_entry	
	.ent	kernel_entry
	.type	kernel_entry, @function
kernel_entry:
	addiu sp, sp, -24
	sw ra, 0(sp)
	bnez $4, 1f
	nop

	jal do_yield
	nop
	beqz $0,2f
	nop

1:
	jal do_exit
	nop
2:
	lw ra, 0(sp)
	addiu sp, sp, 24
	jr ra
	nop

	.align	2
	.globl  scheduler_entry	
	.ent	scheduler_entry
	.type	scheduler_entry, @function
scheduler_entry:
	# call scheduler, which will set new current process
	# need student add
	la t0, scheduler
	jal t0
	lw t1,(current_running)
	lw $16, 0(t1)
	lw $17, 4(t1)
	lw $18, 8(t1)
	lw $19, 12(t1)
	lw $20, 16(t1)
	lw $21, 20(t1)
	lw $22, 24(t1)
	lw $23, 28(t1)
	lw $29, 32(t1)
	lw $30, 36(t1)
	lw $31, 40(t1)
	jr ra
	.align	2
	.globl  save_pcb	
	.ent	save_pcb
	.type	save_pcb, @function
save_pcb:
	# save the pcb of the currently running process
	# need student add
	lw t1,(current_running)	
	lw t2,16(sp)
	sw t2, 40(t1)
	addiu sp,sp,24
	sw sp, 32(t1)
4:	
	sw $16, 0(t1)
	sw $17, 4(t1)
	sw $18, 8(t1)
	sw $19, 12(t1)
	sw $20, 16(t1)
	sw $21, 20(t1)
	sw $22, 24(t1)
	sw $23, 28(t1)
	sw $30, 36(t1)
	jr ra
