.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
BB_0:
    # x86 IRInstr: ldconst %eax 17    movl $17, %eax

    # x86 IRInstr: copy x %eax    movl %eax, x

    # x86 IRInstr: ldconst %eax 42    movl $42, %eax

    # x86 IRInstr: copy y %eax    movl %eax, y

    # x86 IRInstr: ldconst %eax 65    movl $65, %eax

    # x86 IRInstr: copy !tmp0 %eax    movl %eax, !tmp0

    # x86 IRInstr: ldconst %eax 1    movl $1, %eax

    # x86 IRInstr: add %eax !tmp0 %eax    movl !tmp0, %eax
    addl %eax, %eax

    # x86 IRInstr: copy %rdi %eax    movl %eax, %rdi

    # x86 IRInstr: call putchar %eax %rdi    # Opération non implémentée

    # x86 IRInstr: copy ret %eax    movl %eax, ret

    # x86 IRInstr: copy %eax ret    movl ret, %eax

    # Fin de bloc (terminal)
    popq %rbp
    ret
