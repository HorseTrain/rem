#ifndef X86_PIPELINE
#define X86_PIPELINE

#include "ir/ir.h"
#include "abi_information.h"

void assemble_x86_64_pipeline(void** result_code, uint64_t* result_code_size, ir_operation_block* source_ir, bool optimize, abi working_abi);

#endif