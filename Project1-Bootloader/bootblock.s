	.text
	.globl main

main:
	# check the offset of main
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	
	li $4,0xa0800200
	li $5,0x200
	lw $6,os_size
	move $10,$31
	li $8,0x8007b1a8
	jalr $8
	li $9,0xa080026c
	jalr $9
	#move $31,10
	#li $8,0x8007b1a8
	#li $9,0xa080026c
	#jr $8
	#jr $9

	nop
	nop
	jr $31
	#need add code
	#read kernel

	.data
	os_size:	.word 0x00000001

