.globl main
 main:
    pushq %rbp
    movq %rsp, %rbp
    movl $17, %eax
    movl %eax, -4(%rbp)
    movl $42, %eax
    movl %eax, -8(%rbp)
    movl $65, %eax
    pushq %rax
    movl $1, %eax
    popq %rcx
    add %rcx, %rax
    movq %rax, %rdi
    call putchar
    movl %eax, -12(%rbp)
    movl -12(%rbp), %eax
    popq %rbp
    ret
