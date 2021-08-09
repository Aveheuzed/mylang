#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "compiler/altvm/bf.h"

typedef uint8_t Word;

void interpretBF(const Bytecode bytecode[]) {
        #define NEXT() goto *labels[(++text)->control.mode]
        #define NEXT_NOAUTOINC() goto *labels[(text)->control.mode]

        static const void* labels[] = {
                [MODE_COMPUTE] = &&mode_compute,
                [MODE_END] = &&mode_end,
                [MODE_IN] = &&mode_in,
                [MODE_OUT] = &&mode_out,
                [MODE_CJUMPFWD] = &&mode_cjumpfwd,
                [MODE_NCJUMPFWD] = &&mode_ncjumpfwd,
                [MODE_CJUMPBWD] = &&mode_cjumpbwd,
                [MODE_NCJUMPBWD] = &&mode_ncjumpbwd,
        };
        Bytecode const* text = bytecode;
        int8_t *const band = calloc(65536, sizeof(*band));
        unsigned int pos = 0;

        NEXT_NOAUTOINC();

        mode_compute:
        for (unsigned char length = text->control.length; length-- > 0;)
                pos += (++text)->byte,
                band[pos] += (++text)->byte;
        NEXT();

        mode_in:
        band[pos] = getchar();
        NEXT();

        mode_out:
        putchar(band[pos]);
        NEXT();

        mode_cjumpfwd:
        if (!band[pos]) text += text->control.length;
        NEXT();

        mode_ncjumpfwd:
        if (!band[pos]) {
                size_t increment;
                memcpy(&increment, ++text, sizeof(increment));
                text += increment;
        } else text += sizeof(size_t) + 1;
        NEXT_NOAUTOINC();

        mode_cjumpbwd:
        if (band[pos]) text -= text->control.length;
        NEXT();

        mode_ncjumpbwd:
        if (band[pos]) {
                size_t increment;
                memcpy(&increment, ++text, sizeof(increment));
                text -= increment;
        } else text += sizeof(size_t) + 1;
        NEXT_NOAUTOINC();

        mode_end:
        free(band);

        #undef NEXT_NOAUTOINC
        #undef NEXT
}
