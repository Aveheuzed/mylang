#ifndef bf_h
#define bf_h

#include "pipeline/bytecode.h"

void interpretBF(CompressedBFOperator const* text, const CompressedBFOperator* stop_text);

#endif
