#ifndef bf_h
#define bf_h

#include "compiler/compile/bytecode.h"

void interpretBF(CompressedBFOperator const* text, const CompressedBFOperator* stop_text);

#endif
