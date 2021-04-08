# pdata:     %r12
# pos:       %r13
# data_len at (rsp)
# ptext:     %r14
# stop_text: %r15
# jumptable : %rbx

# bytecode :
# operator = 3 lsb
# run = 5 msb

        .text

.macro EXTRACT_OPERATOR
        andb $7, %al
.endm

.macro EXTRACT_RUN
        shrb $3, %al
.endm

.macro NEXT
        cmpq %r14, %r15
        jna .end
        movzbq (%r14), %rax
        EXTRACT_OPERATOR
        jmpq *(%rbx, %rax, 8)
.endm

        .globl	interpretBF
        .type	interpretBF, @function

interpretBF:
        # text comes in %rdi
        # stop_text comes in %rsi
        cmpq %rdi, %rsi
        ja .interpretBF
        ret

.interpretBF: # real start!!
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15
        pushq %rbp
        pushq %rbx

        pushq $0xffff # data_len

        movq %rsi, %r15
        movq %rdi, %r14
        xorq %r13, %r13


        movq $0, %rdi
        movq $0, %rsi
        movq (%rsp), %rdx
        call growBand
        movq %rax, %r12

        leaq jumptable(%rip), %rbx
        NEXT
.op_plus:
        movb (%r14), %al
        EXTRACT_RUN
        addb %al, (%r12, %r13)
        incq %r14
        NEXT
.op_minus:
        movb (%r14), %al
        EXTRACT_RUN
        subb %al, (%r12, %r13)
        incq %r14
        NEXT
.op_left:
        movzbq (%r14), %rax
        EXTRACT_RUN
        subq %rax, %r13
        incq %r14
        NEXT
.op_right:
        movzbq (%r14), %rax
        EXTRACT_RUN
        addq %rax, %r13
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
.op_lbr:
        cmpb $0, (%r12, %r13)
        jne .nojump
        movzbq (%r14), %rax
        EXTRACT_RUN
        jz .lbr_longjump
        addq %rax, %r14
        incq %r14
        NEXT
.lbr_longjump:
        incq %r14
        movq (%r14), %rax
        addq %rax, %r14
        NEXT
.op_rbr:
        cmpb $0, (%r12, %r13)
        je .nojump
        movzbq (%r14), %rax
        EXTRACT_RUN
        jz .rbr_longjump
        subq %rax, %r14
        incq %r14
        NEXT
.rbr_longjump:
        incq %r14
        movq (%r14), %rax
        subq %rax, %r14
        NEXT
.nojump:
        movzbq (%r14), %rax
        EXTRACT_RUN
        jnz .nojump_short
        addq $8, %r14
.nojump_short:
        incq %r14
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
        .quad .op_plus
        .quad .op_minus
        .quad .op_left
        .quad .op_right
        .quad .op_in
        .quad .op_out
        .quad .op_lbr
        .quad .op_rbr

.size jumptable, .-jumptable
