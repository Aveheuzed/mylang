#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "pipeline/bf.h"

typedef uint8_t Word;

void interpretBF(const Bytecode bytecode[]) {
        Bytecode const* text = bytecode;
        uint8_t *const band = calloc(65536, sizeof(*band));
        size_t pos = 0;

        for(;;text++) switch (text->control.mode) {
                case MODE_COMPUTE:
                        for (unsigned char length = text->control.length; length-- > 0;)
                                pos += (++text)->byte,
                                band[pos] += (++text)->byte;
                        break;
                case MODE_END:
                        free(band);
                        return;
                case MODE_IN:
                        band[pos] = getchar();
                        break;
                case MODE_OUT:
                        putchar(band[pos]);
                        break;
                case MODE_CJUMPFWD:
                        if (!band[pos]) text += text->control.length;
                        break;
                case MODE_NCJUMPFWD:
                        if (!band[pos]) {
                                size_t increment;
                                memcpy(&increment, text+1, sizeof(increment));
                                text += increment;
                        } else text += sizeof(size_t);
                        break;
                case MODE_CJUMPBWD:
                        if (band[pos]) text -= text->control.length;
                        break;
                case MODE_NCJUMPBWD:
                        if (band[pos]) {
                                size_t increment;
                                memcpy(&increment, text+1, sizeof(increment));
                                text -= increment;
                        } else text += sizeof(size_t);
                        break;
        }
}
