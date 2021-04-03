#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "pipeline/bf.h"

typedef uint8_t Word;

static inline Word* growBand(Word* ptr, const size_t oldlen, const size_t newlen) {
        ptr = reallocarray(ptr, newlen, sizeof(Word));
        memset(ptr+oldlen, 0, (newlen-oldlen)*sizeof(Word));
        return ptr;
}

void interpretBF(const CompiledProgram* bytecode) {
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

        if (!bytecode->len) return;

        CompressedBFOperator const* text = bytecode->bytecode;
        const CompressedBFOperator* stop_text = text + bytecode->len;

        #define NEXT() goto *((++text < stop_text) ? labels[text->operator] : &&end)

        size_t pos = 0;
        size_t len = 1;
        Word* data = growBand(NULL, 0, 1);

        goto *(labels[text->operator]);

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
                        if (text->run) text += text->run;
                        else {
                                size_t increment;
                                memcpy(&increment, text+1, sizeof(size_t));
                                text += increment;
                        }
                } else if (!text->run) text += sizeof(size_t)/sizeof(*text);
                NEXT();
        rbracket:
                if (data[pos]) {
                        if (text->run) text -= text->run;
                        else {
                                size_t increment;
                                memcpy(&increment, text+1, sizeof(size_t));
                                text -= increment;
                        }
                } else if (!text->run) text += sizeof(size_t)/sizeof(*text);
                NEXT();

        end:
        free(data);

        #undef NEXT
}
