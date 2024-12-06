

#ifndef X86_PRE_ALLOCATOR_H
#define X86_PRE_ALLOCATOR_H

#include "ir/ir.h"
#include "abi_information.h"

#define RAX(size) ir_operand::create_hardware_reg(0, size)
#define RCX(size) ir_operand::create_hardware_reg(1, size)
#define RDX(size) ir_operand::create_hardware_reg(2, size)
#define RSP(size) ir_operand::create_hardware_reg(4, size)

struct x86_pre_allocator_context
{
	uint64_t																scrap_index;

	arena_allocator*														allocator;
	ir_operation_block*														ir;
	ir_operation_block*														source_ir;
	intrusive_linked_list<intrusive_linked_list_element<ir_operation>*>*	revisit_instructions;

	int																		opernad_counts[2];

	static void run_pass(x86_pre_allocator_context* pre_allocator_context, ir_operation_block* result_ir, ir_operation_block* source, cpu_information cpu_data, os_information os);
};

#endif