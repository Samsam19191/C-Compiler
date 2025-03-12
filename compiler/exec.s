.globl main
 main: 
    pushq %rbp 
    movq %rsp, %rbp 
    movl $2, %eax
    pushq %rax
    movl $4, %eax
    popq %rcx
    imul %rcx, %rax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    popq %rbp
    ret
