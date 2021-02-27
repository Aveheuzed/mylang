#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "bf.h"

static inline Word* growBand(Word* ptr, const size_t oldlen, const size_t newlen) {
        ptr = reallocarray(ptr, newlen, sizeof(Word));
        memset(ptr+oldlen, 0, (newlen-oldlen)*sizeof(Word));
        return ptr;
}

static CompressedBFOperator* computeJumps(CompressedBFOperator* bytecode) {
        CompressedBFOperator* jump_forward_from;
        CompressedBFOperator* jump_backward_from;
        CompressedBFOperator const* jump_forward_to;
        CompressedBFOperator const* jump_backward_to;

        for (; bytecode->run && bytecode->operator != BF_JUMP_FWD; bytecode++) {
                if (bytecode->operator == BF_JUMP_BWD) {
                        return NULL;
                }
        }

        if (!bytecode->run) return bytecode;
        if (bytecode->run != 1) LOG("Error : compressed brackets detected");

        bytecode++;
        jump_forward_from = bytecode;
        bytecode += sizeof(ptrdiff_t)/sizeof(CompressedBFOperator);
        jump_backward_to = bytecode;


        for (; bytecode->run && bytecode->operator != BF_JUMP_BWD; bytecode++) {
                if (bytecode->operator == BF_JUMP_FWD) {
                        bytecode = computeJumps(bytecode);
                        if (bytecode == NULL) return NULL;
                        else bytecode--;
                }
        }

        if (!bytecode->run) return NULL;
        if (bytecode->run != 1) LOG("Error : compressed brackets detected");

        bytecode++;
        jump_backward_from = bytecode;
        bytecode += sizeof(ptrdiff_t)/sizeof(CompressedBFOperator);
        jump_forward_to = bytecode;

        ptrdiff_t forward_jump = jump_forward_to - jump_forward_from;
        ptrdiff_t backward_jump = jump_backward_to - jump_backward_from;

        memcpy(jump_forward_from, &forward_jump, sizeof(forward_jump));
        memcpy(jump_backward_from, &backward_jump, sizeof(backward_jump));

        return bytecode;
}

static CompressedBFOperator* prepare_bytecode(FILE* file) {
        size_t len = 16;
        size_t index = 0;
        CompressedBFOperator* result = calloc(len, sizeof(CompressedBFOperator));

        initialize:
        {
                BFOperator operator;
                int byte = getc(file);
                switch (byte) {
                        case '+': operator = BF_PLUS; break;
                        case '-': operator = BF_MINUS; break;
                        case '.': operator = BF_OUTPUT; break;
                        case ',': operator = BF_INPUT; break;
                        case '<': operator = BF_LEFT; break;
                        case '>': operator = BF_RIGHT; break;
                        case '[': operator = BF_JUMP_FWD; break;
                        case ']': operator = BF_JUMP_BWD; break;
                        case EOF: return result; // correctly initialized thanks to calloc()
                        default: goto initialize;
                }
                result[index++] = (CompressedBFOperator) {.operator=operator, .run=1};
        }

        // actual loop
        for (int byte = getc(file); byte != EOF; byte = getc(file)) {
                BFOperator operator;
                if (index >= len) {
                        len *= 2;
                        result = reallocarray(result, len, sizeof(CompressedBFOperator));
                }
                switch (byte) {
                        case '+': operator = BF_PLUS; break;
                        case '-': operator = BF_MINUS; break;
                        case '.': operator = BF_OUTPUT; break;
                        case ',': operator = BF_INPUT; break;
                        case '<': operator = BF_LEFT; break;
                        case '>': operator = BF_RIGHT; break;
                        case '[': operator = BF_JUMP_FWD; break;
                        case ']': operator = BF_JUMP_BWD; break;
                        default : continue; // continue the `for` // break the `switch`
                }
                if (
                        operator <= BF_CANCOMPRESS
                        && operator == result[index-1].operator
                        && result[index-1].run < BF_MAX_RUN
                ) {
                        result[index-1].run++;
                }
                else {
                        result[index++] = (CompressedBFOperator) {.operator=operator, .run=1};
                        if (operator > BF_NOOPERAND) {
                                index += sizeof(ptrdiff_t)/sizeof(*result);
                                // to prevent the next operand to nest here (`+`)
                                result[index-1].run = BF_MAX_RUN;
                        }
                }

        }

        if (index >= len) {
                len = index+1;
                result = reallocarray(result, len, sizeof(CompressedBFOperator));
        }

        result[index].run = 0;

        for (
                CompressedBFOperator* ptr=result;
                ptr < (result + index);
                ptr = computeJumps(ptr)
        ) {
                if (ptr == NULL) {
                        free(result);
                        return NULL;
                }
        }

        return result;
}

void interpretBF(CompressedBFOperator const* text) {
        static const void* labels[] = {
                [BF_RIGHT] = &&rsh,
                [BF_LEFT] = &&lsh,
                [BF_PLUS] = &&inc,
                [BF_MINUS] = &&dec,
                [BF_OUTPUT] = &&out,
                [BF_INPUT] = &&in,
                [BF_JUMP_FWD] = &&lbracket,
                [BF_JUMP_BWD] = &&rbracket,
        };

        #define NEXT() goto *((++text)->run ? labels[text->operator] : &&end)

        size_t pos = 0;
        size_t len = 1;
        Word* data = growBand(NULL, 0, 1);

        goto *(text->run ? labels[text->operator] : &&end);

        inc:
                data[pos] += text->run;
                NEXT();
        dec:
                data[pos] -= text->run;
                NEXT();
        lsh:
                pos -= text->run;
                NEXT();
        rsh:
                pos += text->run;
                if (pos>=len) {
                        data = growBand(data, len, pos*2);
                        len = pos*2;
                }
                NEXT();
        in:
                data[pos] = getchar();
                NEXT();
        out:
                putchar(data[pos]);
                NEXT();
        lbracket:
                if (!data[pos]) {
                        ptrdiff_t increment;
                        memcpy(&increment, text+1, sizeof(ptrdiff_t));
                        text += increment;
                } else text += sizeof(ptrdiff_t)/sizeof(*text);
                NEXT();
        rbracket:
                if (data[pos]) {
                        ptrdiff_t increment;
                        memcpy(&increment, text+1, sizeof(ptrdiff_t));
                        text += increment;
                } else text += sizeof(ptrdiff_t)/sizeof(*text);
                NEXT();

        end:
        free(data);

        #undef NEXT
}


void main(const int argc, const char* argv[]) {
        if (argc != 2) {
                fprintf(stderr, "Bad usage: expected one argument: brainfuck file.\n");
                exit(EXIT_FAILURE);
        }
        FILE* file = fopen(argv[1], "r");
        CompressedBFOperator* code = prepare_bytecode(file);
        fclose(file);
        if (code != NULL) interpretBF(code);
        free(code);
}
