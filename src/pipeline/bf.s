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

.macro EXTRACT_OPERATOR target=%dl
        andb $7, \target
.endm

.macro EXTRACT_RUN target=%al
        shrb $3, \target
.endm

.macro NEXT
        # results in bytecode in %rax (%al actually)
        # and %rdx clobbered
        cmpq %r14, %r15
        jna .end
        movzbq (%r14), %rax
        movq %rax, %rdx
        EXTRACT_OPERATOR
        jmpq *(%rbx, %rdx, 8)
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

        xorq %rdi, %rdi
        xorq %rsi, %rsi
        movq (%rsp), %rdx
        call growBand
        movq %rax, %r12

        leaq jumptable(%rip), %rbx
        NEXT

.op_plus:
        EXTRACT_RUN
        addb %al, (%r12, %r13)
        incq %r14
        NEXT
.op_minus:
        EXTRACT_RUN
        subb %al, (%r12, %r13)
        incq %r14
        NEXT
.op_left:
        EXTRACT_RUN
        subq %rax, %r13
        incq %r14
        NEXT
.op_right:
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

# -------------------------------------lbr--------------------------------------
.op_lbr:
        # cmpb $0, (%r12, %r13)
        # Z → data[pos] == 0
        # EXTRACT_RUN
        # Z → run == 0
        # 00: data[pos] != 0 ; run != 0 → nojump, short → .lbr_nojump_short
        # 01: data[pos] != 0 ; run == 0 → nojump, long  → .lbr_nojump_long
        # 10: data[pos] == 0 ; run != 0 →   jump, short → .lbr_shortjump
        # 11: data[pos] == 0 ; run == 0 →   jump, long  → .lbr_longjump
        cmpb $0, (%r12, %r13)
        # jz .lbr_jump
        jnz .lbr_nojump

.lbr_jump:
        EXTRACT_RUN
        # jz .lbr_jump_long
        jnz .lbr_jump_short

.lbr_jump_long:
        incq %r14
        addq (%r14), %r14
        NEXT
.lbr_jump_short:
        incq %r14
        addq %rax, %r14
        NEXT

.lbr_nojump:
        EXTRACT_RUN
        jz .lbr_nojump_long
        # jnz .lbr_nojump_short

# ------------------------------------- common to lbr and rbr

.lbr_nojump_short:
.rbr_nojump_short:
        incq %r14
        NEXT

# -------------------------------------rbr--------------------------------------

.op_rbr:
        # cmpb $0, (%r12, %r13)
        # Z → data[pos] == 0
        # EXTRACT_RUN
        # Z → run == 0
        # 00: data[pos] != 0 ; run != 0 →   jump, short → .rbr_shortjump
        # 01: data[pos] != 0 ; run == 0 →   jump, long  → .rbr_longjump
        # 10: data[pos] == 0 ; run != 0 → nojump, short → .rbr_nojump_short
        # 11: data[pos] == 0 ; run == 0 → nojump, long  → .rbr_nojump_long
        cmpb $0, (%r12, %r13)
        jz .rbr_nojump
        # jnz .rbr_jump

.rbr_jump:
        EXTRACT_RUN
        # jz .rbr_jump_long
        jnz .rbr_jump_short

.rbr_jump_long:
        incq %r14
        subq (%r14), %r14
        NEXT
.rbr_jump_short:
        incq %r14
        subq %rax, %r14
        NEXT

.rbr_nojump:
        EXTRACT_RUN
        # jz .rbr_nojump_long
        jnz .rbr_nojump_short

# ------------------------------------- common to lbr and rbr

.lbr_nojump_long:
.rbr_nojump_long:
        addq $9, %r14
        NEXT

# ------------------------------------------------------------------------------

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
