#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t Word;
const Word LIM = UINT8_MAX;

static inline Word* growBand(Word* ptr, const size_t oldlen, const size_t newlen) {
        ptr = reallocarray(ptr, newlen, sizeof(Word));
        memset(ptr+oldlen, 0, (newlen-oldlen)*sizeof(Word));
        return ptr;
}

static char* prepare_bytecode(FILE* file) {
        size_t len = 1;
        size_t index = 0;
        char* result = calloc(len, sizeof(char));
        for (int byte = getc(file); byte != EOF; byte = getc(file)) {
                if (index >= len) {
                        result = reallocarray(result, len*=2, sizeof(char));
                }
                if (strchr("+-[]<>.,", byte) != NULL) {
                        result[index++] = byte;
                }
        }
        result[index] = '\0';
        return result;
}

void interpretBF(char const* text) {
        static const void* labels[] = {
                ['<'] = &&rsh,
                ['>'] = &&lsh,
                ['+'] = &&inc,
                ['-'] = &&dec,
                ['.'] = &&out,
                [','] = &&in,
                ['['] = &&lbracket,
                [']'] = &&rbracket,
                ['\0'] = &&end,
        };

        #define NEXT() goto *labels[*++text]

        size_t pos = 0;
        size_t len = 1;
        Word* data = growBand(NULL, 0, 1);

        goto *labels[*text];

        inc:
                data[pos]++;
                NEXT();
        dec:
                data[pos]--;
                NEXT();
        rsh:
                pos--;
                NEXT();
        lsh:
                pos++;
                if (pos>=len) {
                        data = growBand(data, len, len*2);
                        len*=2;
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
                        for(text++; (*text != ']' || stack != 0); text++) {
                                if (*text == '[') stack++;
                                if (*text == ']') stack--;
                        }
                }
                NEXT();
        rbracket:
                if (data[pos]) {
                        size_t stack = 0;
                        for(text--; (*text != '[' || stack != 0); text--) {
                                if (*text == '[') stack--;
                                if (*text == ']') stack++;
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
        char* code = prepare_bytecode(file);
        interpretBF(code);
        free(code);
}
