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

    # save $31
    addiu $29, $29, -4
    sw $31, ($29)
    
    li $4, 0xa0800200
    li $5, 0x200
    li $6, 0x12345678
    
    jal 0x8007b1a8
    nop
    jal 0xa0800200
    nop
    
    # restore $31
    lw $31, ($29)
    addiu $29, $29, 4
    jr $31
    nop
