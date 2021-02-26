#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bf.h"

static inline Word* growBand(Word* ptr, const size_t oldlen, const size_t newlen) {
        ptr = reallocarray(ptr, newlen, sizeof(Word));
        memset(ptr+oldlen, 0, (newlen-oldlen)*sizeof(Word));
        return ptr;
}

static CompressedBFOperator* prepare_bytecode(FILE* file) {
        size_t len = 1;
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
                }

        }
        result[index].run = 0;
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
                        size_t stack = 0;
                        for(text++; (text->operator != BF_JUMP_BWD || stack != 0); text++) {
                                if (text->operator == BF_JUMP_FWD) stack++;
                                if (text->operator == BF_JUMP_BWD) stack--;
                        }
                }
                NEXT();
        rbracket:
                if (data[pos]) {
                        size_t stack = 0;
                        for(text--; (text->operator != BF_JUMP_FWD || stack != 0); text--) {
                                if (text->operator == BF_JUMP_FWD) stack--;
                                if (text->operator == BF_JUMP_BWD) stack++;
                        }
                }
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
        interpretBF(code);
        free(code);
}
