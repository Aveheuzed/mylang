# pdata:     %r12
# pos:       %r13
# ptext:     %r14
# jumptable : %rbx

# bytecode :
# operator = 3 lsb
# run = 5 msb

        .text

.macro EXTRACT_OPERATOR target=%dl
        andb $7, \target
.endm

.macro EXTRACT_RUN target=%cl
        shrb $3, \target
.endm

.macro NEXT
        # results in run in %rcx (%cl actually)
        # and %rdx clobbered (containing operator)
        movzbq (%r14), %rcx
        movq %rcx, %rdx
        EXTRACT_RUN
        EXTRACT_OPERATOR
        jz .op_compute # most common operation, easy to detect → effective shortcut
        jmpq *(%rbx, %rdx, 8)
.endm

        .globl	interpretBF
        .type	interpretBF, @function

interpretBF:
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15
        pushq %rbp
        pushq %rbx

        subq $8, %rsp

        movq %rdi, %r14
        xorq %r13, %r13

        movq $65536, %rdi
        movq $1, %rsi
        call calloc@PLT
        movq %rax, %r12

        leaq jumptable(%rip), %rbx
        NEXT

# ------------------------------------------------------------------------------
.op_compute:
        movsbq 1(%r14), %rdx # <>
        movb 2(%r14), %al # +-

        addq %rdx, %r13
        addb %al, (%r12, %r13)

        addq $2, %r14
        decb %cl
        jnz .op_compute
        incq %r14
        NEXT

.op_in:
        call getchar@PLT
        movb %al, (%r12, %r13)
        incq %r14
        NEXT
.op_out:
        movzbq (%r12, %r13), %rdi
        call putchar@PLT
        incq %r14
        NEXT

.op_clbr:
        incq %r14
        cmpb $0, (%r12, %r13)
        jne .clbr_nojump
        addq %rcx, %r14
.clbr_nojump:
        NEXT

.op_nclbr:
        incq %r14
        cmpb $0, (%r12, %r13)
        jne .nclbr_nojump
        addq (%r14), %r14
        NEXT
.nclbr_nojump:
        addq $8, %r14
        NEXT

.op_crbr:
        incq %r14
        cmpb $0, (%r12, %r13)
        je .crbr_nojump
        subq %rcx, %r14
.crbr_nojump:
        NEXT

.op_ncrbr:
        incq %r14
        cmpb $0, (%r12, %r13)
        je .ncrbr_nojump
        subq (%r14), %r14
        NEXT
.ncrbr_nojump:
        addq $8, %r14
        NEXT

.end:
        movq %r12, %rdi

        addq $8, %rsp
        popq %rbx
        popq %rbp
        popq %r15
        popq %r14
        popq %r13
        popq %r12


        jmp free@PLT

.size	interpretBF, .-interpretBF

jumptable:
        .quad .op_compute
        .quad .end
        .quad .op_in
        .quad .op_out
        .quad .op_clbr
        .quad .op_nclbr
        .quad .op_crbr
        .quad .op_ncrbr

.size jumptable, .-jumptable
