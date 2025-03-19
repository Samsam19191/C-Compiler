.att_syntax prefix
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
BB_0:
    # x86 IRInstr: ldconst %eax 2
    movl $2, %eax

    # x86 IRInstr: copy !tmp0 %eax
    movl %eax, !tmp0

    # x86 IRInstr: ldconst %eax 2
    movl $2, %eax

    # x86 IRInstr: add %eax !tmp0 %eax
    movl !tmp0, %eax
    addl %eax, %eax

    # Fin de bloc (terminal)
    popq %rbp
    ret
