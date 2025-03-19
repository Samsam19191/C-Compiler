.att_syntax prefix
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
BB_0:
    # x86 IRInstr: ldconst %eax 82
    movl $82, %eax

    # x86 IRInstr: copy %rdi %eax
    movl %eax, %rdi

    # x86 IRInstr: call putchar %eax %rdi
    call putchar

    # x86 IRInstr: copy ret %eax
    movl %eax, ret

    # x86 IRInstr: copy %eax ret
    movl ret, %eax

    # Fin de bloc (terminal)
    popq %rbp
    ret
