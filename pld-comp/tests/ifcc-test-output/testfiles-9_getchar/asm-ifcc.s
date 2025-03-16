.globl main
 main:
    pushq %rbp
    movq %rsp, %rbp
    call getchar
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax
    popq %rbp
    ret
