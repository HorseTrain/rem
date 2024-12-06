

#ifndef IR_H
#define IR_H

#include <unordered_map>
#include <string>

#include "memory_tools.h"

enum ir_instructions : uint64_t
{
	ir_add,
	ir_bitwise_and,
	ir_bitwise_exclusive_or,
	ir_bitwise_not,
	ir_bitwise_or,
	ir_close_and_return,
	ir_compare_and_swap,
	ir_compare_equal,
	ir_compare_greater_equal_signed,
	ir_compare_greater_equal_unsigned,
	ir_compare_greater_signed,
	ir_compare_greater_unsigned,
	ir_compare_less_equal_signed,
	ir_compare_less_equal_unsigned,
	ir_compare_less_signed,
	ir_compare_less_unsigned,
	ir_compare_not_equal,
	ir_divide_signed,
	ir_divide_unsigned,
	ir_get_argument,
	ir_incrament,
	ir_jump_if,
	ir_load,
	ir_mark_label,
	ir_move,
	ir_multiply,
	ir_multiply_hi_signed,
	ir_multiply_hi_unsigned,
	ir_negate,
	ir_no_operation,
	ir_open_context,
	ir_register_allocator_p_lock,
	ir_register_allocator_p_unlock,
	ir_shift_left,
	ir_shift_right_signed,
	ir_shift_right_unsigned,
	ir_rotate_right,
	ir_store,
	ir_subtract,
	ir_sign_extend,
	ir_double_shift_right,
	x86_cqo,
	x86_cdq,
	x86_cwd,
	arm_load_context,
	arm_store_context,
};

static std::string instruction_names[] = {
	"ir_add",
	"ir_bitwise_and",
	"ir_bitwise_exclusive_or",
	"ir_bitwise_not",
	"ir_bitwise_or",
	"ir_close_and_return",
	"ir_compare_and_swap",
	"ir_compare_equal",
	"ir_compare_greater_equal_signed",
	"ir_compare_greater_equal_unsigned",
	"ir_compare_greater_signed",
	"ir_compare_greater_unsigned",
	"ir_compare_less_equal_signed",
	"ir_compare_less_equal_unsigned",
	"ir_compare_less_signed",
	"ir_compare_less_unsigned",
	"ir_compare_not_equal",
	"ir_divide_signed",
	"ir_divide_unsigned",
	"ir_get_argument",
	"ir_incrament",
	"ir_jump_if",
	"ir_load",
	"ir_mark_label",
	"ir_move",
	"ir_multiply",
	"ir_multiply_hi_signed",
	"ir_multiply_hi_unsigned",
	"ir_negate",
	"ir_no_operation",
	"ir_open_context",
	"ir_register_allocator_p_lock",
	"ir_register_allocator_p_unlock",
	"ir_shift_left",
	"ir_shift_right_signed",
	"ir_shift_right_unsigned",
	"ir_rotate_right",
	"ir_store",
	"ir_subtract",
	"ir_sign_extend",
	"ir_double_shift_right",
	"x86_cqo",
	"x86_cdq",
	"arm_load_context",
	"arm_store_context"
};

enum ir_operand_meta : uint64_t
{
	int8,
	int16,
	int32,
	int64,
	int128,

	top,

	is_hardware_register	= 1ULL << 32,
	is_constant				= 1ULL << 33
};

struct ir_operand
{
	uint64_t value;
	uint64_t meta_data;

	static bool			is_register(ir_operand* test);
	static bool			is_constant(ir_operand* test);
	static bool			is_hardware(ir_operand* test);
	static bool			is_vector(ir_operand* test);

	static uint64_t		get_masked_constant(ir_operand* test);

	static uint64_t		get_raw_size(ir_operand* test);
	static uint64_t		get_meta(ir_operand* test);

	static void			set_raw_size(ir_operand* test, uint64_t new_size);

	static ir_operand	create_con(uint64_t value, uint64_t size = ir_operand_meta::int64);
	static ir_operand	create_reg(uint64_t value, uint64_t size = ir_operand_meta::int64);
	static ir_operand	create_hardware_reg(uint64_t value, uint64_t size = ir_operand_meta::int64);
	static ir_operand	copy_new_raw_size(ir_operand source, uint64_t new_size);

	static bool 		are_equal(ir_operand left, ir_operand right);
	static bool 		same_size(ir_operand left, ir_operand right);
};

struct ir_operation
{
	uint64_t				instruction;
	fast_array<ir_operand>	destinations;
	fast_array<ir_operand>	sources;
};

struct ir_operation_block
{
	arena_allocator*						allocator;
	intrusive_linked_list<ir_operation>*	operations;

	static void								create(ir_operation_block** result,arena_allocator* allocator);
	static void								create_raw_operation(arena_allocator* allocator, ir_operation* result, uint64_t instruction, int destination_count, int source_count);
	
	static intrusive_linked_list_element<ir_operation>* emit(ir_operation_block* block,ir_operation operation, intrusive_linked_list_element<ir_operation>* point = nullptr);

	static void create_vector_gp_remap_scheme(arena_allocator* allocator, std::unordered_map<uint64_t, uint64_t>* remap_store, std::unordered_map<uint64_t, uint64_t>** remap_redirect);
	static void clamp_operands(ir_operation_block* ir, bool use_bit_register_allocations, int* size_counts = nullptr);

	static intrusive_linked_list_element<ir_operation>* emit_with(ir_operation_block* ir, uint64_t instruction, ir_operand* destinations, int destination_count, ir_operand* sources, int source_count, intrusive_linked_list_element<ir_operation>* point = nullptr);

	static intrusive_linked_list_element<ir_operation>* emitds(ir_operation_block* ir, uint64_t instruction, ir_operand destination_0, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emitds(ir_operation_block* ir, uint64_t instruction, ir_operand destination_0, ir_operand source_0, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emitds(ir_operation_block* ir, uint64_t instruction, ir_operand destination_0, ir_operand source_0, ir_operand source_1, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emitds(ir_operation_block* ir, uint64_t instruction, ir_operand destination_0, ir_operand source_0, ir_operand source_1, ir_operand source_2, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emitds(ir_operation_block* ir, uint64_t instruction, ir_operand destination_0, ir_operand source_0, ir_operand source_1, ir_operand source_2, ir_operand source_3, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emitds(ir_operation_block* ir, uint64_t instruction, ir_operand destination_0, ir_operand source_0, ir_operand source_1, ir_operand source_2, ir_operand source_3, ir_operand source_4, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emits(ir_operation_block* ir, uint64_t instruction, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emits(ir_operation_block* ir, uint64_t instruction, ir_operand source_0, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emits(ir_operation_block* ir, uint64_t instruction, ir_operand source_0, ir_operand source_1, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emits(ir_operation_block* ir, uint64_t instruction, ir_operand source_0, ir_operand source_1, ir_operand source_2, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emits(ir_operation_block* ir, uint64_t instruction, ir_operand source_0, ir_operand source_1, ir_operand source_2, ir_operand source_3, intrusive_linked_list_element<ir_operation>* point = nullptr);
	static intrusive_linked_list_element<ir_operation>* emits(ir_operation_block* ir, uint64_t instruction, ir_operand source_0, ir_operand source_1, ir_operand source_2, ir_operand source_3, ir_operand source_4, intrusive_linked_list_element<ir_operation>* point = nullptr);

	static bool is_label(uint64_t instruction);
	static bool is_label(ir_operation* operation);

	static std::string get_block_log(ir_operation_block* ir);
	static void log(ir_operation_block* ctx);
};

struct ir_control_flow_node
{
	intrusive_linked_list_element<ir_operation>* entry_instruction;
	intrusive_linked_list_element<ir_operation>* final_instruction;
};

struct ir_control_flow_graph
{
	ir_operation_block*																source_ir;
	intrusive_linked_list<ir_control_flow_node*>*									linier_nodes;

	static ir_control_flow_graph* create(ir_operation_block* source);
};

#endif