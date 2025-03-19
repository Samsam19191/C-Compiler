.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
BB_0:
    # x86 IRInstr: ldconst %eax 42    movl $42, %eax

    # Fin de bloc (terminal)
    popq %rbp
    ret
