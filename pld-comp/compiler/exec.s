.att_syntax prefix
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
BB_0:
    # x86 IRInstr: ldconst %eax 9
    movl $9, %eax

    # x86 IRInstr: copy -4(%rbp) %eax
    movl %eax, -4(%rbp)

    # x86 IRInstr: ldconst %eax 3
    movl $3, %eax

    # x86 IRInstr: copy -8(%rbp) %eax
    movl %eax, -8(%rbp)

    # x86 IRInstr: copy %eax -4(%rbp)
    movl -4(%rbp), %eax

    # x86 IRInstr: copy -0(%rbp) %eax
    movl %eax, -0(%rbp)

    # x86 IRInstr: copy %eax -8(%rbp)
    movl -8(%rbp), %eax

    # x86 IRInstr: copy -4(%rbp) %eax
    movl %eax, -4(%rbp)

    # x86 IRInstr: copy %eax -0(%rbp)
    movl -0(%rbp), %eax

    # x86 IRInstr: unknown -4(%rbp)
    cltd
    idivl -4(%rbp)

    # Fin de bloc (terminal)
    popq %rbp
    ret
