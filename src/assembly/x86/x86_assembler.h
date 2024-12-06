#ifndef X86_ASSEMBLER_H
#define X86_ASSEMBLER_H

#include "ir/ir.h"
#include "abi_information.h"

void assemble_x86_64_code(void** result_code, uint64_t* result_code_size, ir_operation_block* source_ir);
void assemble_x86_abi_caller_code(void* result_code, uint64_t* result_code_size, abi abi_information);

#endif