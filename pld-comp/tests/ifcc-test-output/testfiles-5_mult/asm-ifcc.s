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

    # x86 IRInstr: mul %eax %eax !tmp0
    movl %eax, %eax
    imull !tmp0, %eax

    # Fin de bloc (terminal)
    popq %rbp
    ret
