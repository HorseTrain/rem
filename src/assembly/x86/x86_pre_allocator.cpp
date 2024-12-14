#include "x86_pre_allocator.h"
#include "ir/bit_register_allocations.h"
#include "ir/checks.h"
#include "tools/bit_tools.h"

//I'm pretty sure std::initializer_list does not allocate memory.
static void assert_same_size(std::initializer_list<ir_operand> operands)
{
	int count = operands.size();

	if (count == 0)
		return;

	uint64_t first_size = operands.begin()[0].meta_data & UINT32_MAX;

	for (int i = 1; i < count; ++i)
	{
		uint64_t working_size = operands.begin()[i].meta_data & UINT32_MAX;

		assert(working_size == first_size);
	}
}

static uint64_t get_context_size(uint64_t value)
{
	return (value + 32) & ~0b1111ULL;
}

static void emit_move(x86_pre_allocator_context* context, ir_operand destination, ir_operand source)
{
	assert_same_size({ destination, source });

	if (ir_operand::get_raw_size(&destination) != ir_operand::get_raw_size(&source))
	{
		ir_operation_block::log(context->source_ir);

		throw 0;
	}

	assert(ir_operand::get_raw_size(&destination) == ir_operand::get_raw_size(&source));
	assert(!ir_operand::is_constant(&destination));

	ir_operation_block::emitds(context->ir, ir_instructions::ir_move, destination, source);
}

static ir_operand create_scrap_operand(x86_pre_allocator_context* context, uint64_t size)
{
	uint64_t result = context->scrap_index | bit_register_allocations::scrap_allocation;

	context->scrap_index++;

	return ir_operand::create_reg(result, size & UINT32_MAX);
}

static ir_operand register_or_constant(x86_pre_allocator_context* context, ir_operand* source, bool force_copy = false)
{
	if (ir_operand::is_constant(source))
	{
		ir_operand working_result = create_scrap_operand(context, source->meta_data);

		emit_move(context, working_result, *source);

		return working_result;
	}
	
	if (!force_copy)
		return *source;

	ir_operand copy = create_scrap_operand(context, source->meta_data);
	
	emit_move(context, copy, *source);

	return copy;
}

static ir_operand register_or_constant(x86_pre_allocator_context* context, ir_operand source, bool force_copy = false)
{
	return register_or_constant(context, &source, force_copy);
}

static void emit_throw_exception(x86_pre_allocator_context* context)
{
	ir_operand zero = register_or_constant(context, ir_operand::create_con(0));

	//TODO in the future, i want this to throw an actual exception
	//instead of reading a null pointer LOL
	ir_operation_block::emitds(context->ir, ir_load, zero, zero);
}

static void assert_binary_condition_true(x86_pre_allocator_context* context, ir_instructions condition, ir_operand x, ir_operand y)
{
	assert_same_size({x, y});

	ir_operand condition_result = create_scrap_operand(context, x.meta_data);
	
	x = register_or_constant(context, &x);	
	y = register_or_constant(context, &y);

	ir_operation_block::emitds(context->ir, condition, condition_result, x, y);

	ir_operand end = ir_operation_block::create_label(context->ir);

	ir_operation_block::jump_if(context->ir, end, condition_result);

	emit_throw_exception(context);

	ir_operation_block::mark_label(context->ir, end);
}

static void extend_source(x86_pre_allocator_context* context,ir_operand* source, uint64_t new_size, bool is_signed)
{
	ir_operation_block* ir = context->ir;

	ir_operand temp_location = create_scrap_operand(context, new_size);

	if (is_signed)
	{
		ir_operation_block::emitds(ir, ir_sign_extend, temp_location, *source);
	}
	else
	{
		emit_move(context, temp_location, ir_operand::copy_new_raw_size(*source, new_size));
		ir_operation_block::emitds(ir, ir_bitwise_and, temp_location, temp_location, ir_operand::create_con(255));	
	}

	*source = temp_location;	
}

static void emit_conditional_select(x86_pre_allocator_context* result, ir_operand destination, ir_operand condition, ir_operand source_0, ir_operand source_1)
{
	ir_operand working_destination = destination;

	ir_operand working_condition = register_or_constant(result, &condition);

	ir_operand working_source_0 = register_or_constant(result, &source_0);
	ir_operand working_source_1 = register_or_constant(result, &source_1);

	assert_same_size({ working_destination, working_condition, working_source_0, working_source_1 });

	bool extend_result_8 = false;

	if (working_destination.meta_data == int8)
	{
		working_destination.meta_data = int16;
		working_source_0.meta_data = int16;
		working_source_1.meta_data = int16;
	
		//We don't have to change working_condition's size!

		extend_result_8 = true;
	}

	ir_operation_block::emitds(result->ir, ir_conditional_select, working_destination, working_condition, working_source_0, working_source_1);

	if (extend_result_8)
	{
		working_destination = ir_operand::copy_new_raw_size(working_destination, int64);

		ir_operation_block::emitds(result->ir, ir_bitwise_and, working_destination, working_destination, ir_operand::create_con(255));
	}
}

static void emit_d_n_f_d_n_m(x86_pre_allocator_context* result, uint64_t instruction, ir_operand destination, ir_operand source_0, ir_operand source_1)
{
	assert_same_size({ destination, source_0, source_1 });

	ir_operand working_destination = destination;
	ir_operand working_source_0 = register_or_constant(result, &source_0);
	ir_operand working_source_1 = register_or_constant(result, &source_1);

	ir_operand working_scrap	= create_scrap_operand(result, destination.meta_data);

	//ins destination source_0 source_1

	//mov scrap source_0
	emit_move(result, working_scrap, working_source_0);
	
	//ins scrap source_1
	ir_operation_block::emitds(result->ir, instruction, working_scrap, working_scrap, working_source_1);
	 
	//mov destination scrap
	emit_move(result, working_destination, working_scrap);
}

static void emit_compare(x86_pre_allocator_context* result, uint64_t instruction, ir_operand destination, ir_operand source_0, ir_operand source_1)
{
	assert_same_size({ destination, source_0, source_1 });

	ir_operand working_destination = destination;

	ir_operand working_source_0 = register_or_constant(result, &source_0);
	ir_operand working_source_1 = register_or_constant(result, &source_1);

	ir_operation_block::emitds(result->ir, instruction, working_destination, working_source_0, working_source_1);
}

static void emit_shift(x86_pre_allocator_context* result, uint64_t instruction, ir_operand destination, ir_operand source_0, ir_operand source_1)
{
	assert_same_size({ destination, source_0, source_1 });

	ir_operand working_destination = destination;
	ir_operand working_source_0 = register_or_constant(result, &source_0);
	ir_operand working_source_1 = register_or_constant(result, &source_1);

	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_lock, RCX(ir_operand_meta::int64));

	emit_move(result, RCX(destination.meta_data), working_source_1);
	emit_move(result, working_destination, working_source_0);
	ir_operation_block::emitds(result->ir, instruction, working_destination, working_destination, RCX(ir_operand_meta::int8));

	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_unlock, RCX(ir_operand_meta::int64));
}

static void emit_double_shift_right(x86_pre_allocator_context* result, ir_operand destination, ir_operand source_0, ir_operand source_1, ir_operand shift)
{
	assert_same_size({ destination, source_0, source_1, shift });

	ir_operand working_destination = destination;
	ir_operand working_source_0 = register_or_constant(result, &source_0);
	ir_operand working_source_1 = register_or_constant(result, &source_1);
	ir_operand working_shift = register_or_constant(result, &shift);

	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_lock, RCX(ir_operand_meta::int64));

	ir_operand working_scrap = create_scrap_operand(result, working_destination.meta_data);
	emit_move(result, working_scrap, working_source_0);

	emit_move(result, RCX(working_scrap.meta_data), working_shift);

	if (ir_operand::get_raw_size(&destination) == int8)
	{
		working_destination.meta_data = int16;
		working_source_0.meta_data = int16;
		working_source_1.meta_data = int16;
		working_shift.meta_data = int16;
		working_scrap.meta_data = int16;

		extend_source(result, &working_source_0, int16, false);

		emit_move(result, working_scrap, working_source_1);

		ir_operation_block::emitds(result->ir, ir_shift_left, working_scrap, working_scrap, ir_operand::create_con(8, int16));
		ir_operation_block::emitds(result->ir, ir_bitwise_or, working_scrap, working_scrap, working_source_0);
		ir_operation_block::emitds(result->ir, ir_shift_right_unsigned, working_scrap, working_scrap, RCX(working_scrap.meta_data));
	}
	else
	{
		ir_operation_block::emitds(result->ir, ir_instructions::ir_double_shift_right, working_scrap, working_scrap, working_source_1, RCX(working_scrap.meta_data));
	}

	emit_move(result, working_destination, working_scrap);

	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_unlock, RCX(ir_operand_meta::int64));
}

static void emit_d_f_d_n(x86_pre_allocator_context* result, uint64_t instruction, ir_operand destination, ir_operand source)
{
	assert_same_size({destination, source});
	assert_is_register(destination);

	ir_operand working_destination = destination;
	ir_operand working_source_0 = register_or_constant(result, &source);

	emit_move(result, working_destination, working_source_0);
	ir_operation_block::emitds(result->ir, instruction, working_destination, working_destination);
}

//TODO: This code is nasty, needs to be refactored.
static void emit_big_multiply_divide(x86_pre_allocator_context* result, uint64_t instruction, ir_operand destination, ir_operand source_0, ir_operand source_1)
{
	assert_same_size({ destination, source_0, source_1 });

	ir_operation_block* ir = result->ir;

	ir_operand working_destination = destination;
	ir_operand working_source_0 = register_or_constant(result, &source_0);
	ir_operand working_source_1 = register_or_constant(result, &source_1);

	bool extend_8 = ir_operand::get_raw_size(&working_destination) == int8;

	if (extend_8)
	{
		switch (instruction)
		{
			case ir_divide_signed:
			case ir_multiply_hi_signed:
			{
				extend_source(result, &working_source_0, int16, true);
				extend_source(result, &working_source_1, int16, true);
			}; break;

			default:
			{
				extend_source(result, &working_source_0, int16, false);
				extend_source(result, &working_source_1, int16, false);
			}; break;
		}

		working_destination.meta_data = int16;
	}

	ir_operand ax = RAX(working_destination.meta_data);
	ir_operand dx = RDX(working_destination.meta_data);

	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_lock, ax);
	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_lock, dx);

	emit_move(result, ax, working_source_0);

	uint64_t working_size = ir_operand::get_raw_size(&working_destination);

	if (instruction == ir_divide_signed)
	{
		switch (working_size)
		{
			case int16: ir_operation_block::emitds(ir, ir_instructions::x86_cwd, dx, ax); break;
			case int32: ir_operation_block::emitds(ir, ir_instructions::x86_cdq, dx, ax); break;
			case int64: ir_operation_block::emitds(ir, ir_instructions::x86_cqo, dx, ax); break;
			default: assert(false); throw 0;
		}
	}
	else
	{
		emit_move(result, dx, ir_operand::create_con(0, dx.meta_data));
	}

	ir_operand working_destinations[] = { dx, ax };
	ir_operand working_sources[] = { ax, working_source_1 };

	ir_operation_block::emit_with(ir, instruction, working_destinations, 2, working_sources, 2);

	switch (instruction)
	{
	case ir_multiply_hi_signed:
	case ir_multiply_hi_unsigned:
	{
		if (extend_8)
		{
			ir_operation_block::emitds(result->ir, ir_instructions::ir_shift_right_unsigned, ax, ax, ir_operand::create_con(8, int8));
			emit_move(result, working_destination, ax);
		}
		else
		{
			emit_move(result, working_destination, dx);
		}
	}; break;

	default:
	{
		emit_move(result, working_destination, ax);
	}; break;
	}

	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_unlock, dx);
	ir_operation_block::emits(result->ir, ir_instructions::ir_register_allocator_p_unlock, ax);
}

static void emit_return(x86_pre_allocator_context* result, ir_operand to_return)
{
	int raw_size = ir_operand::get_raw_size(&to_return);

	to_return = register_or_constant(result, &to_return, raw_size != int64);

	to_return.meta_data = int64;

	if (raw_size <= int16)
	{
		ir_operation_block::emitds(result->ir,ir_bitwise_and, to_return, to_return, ir_operand::create_con((1 << (8 << raw_size)) - 1));
	}

	ir_operation_block::emits(result->ir, ir_instructions::ir_close_and_return,to_return , ir_operand::create_con(0));

	intrusive_linked_list< intrusive_linked_list_element<ir_operation>*>::insert_element(result->revisit_instructions, result->ir->operations->last->prev);
}

static void emit_argument_load(x86_pre_allocator_context* result, ir_operation* instruction)
{
	assert_is_register(instruction->destinations[0]);
	assert_is_constant(instruction->sources[0]);

	ir_operation_block::emitds(result->ir, ir_instructions::ir_get_argument, instruction->destinations[0], RSP(ir_operand_meta::int64), ir_operand::create_con(0));

	intrusive_linked_list< intrusive_linked_list_element<ir_operation>*>::insert_element(result->revisit_instructions, result->ir->operations->last->prev);

	ir_operation_block::emitds(result->ir, ir_instructions::ir_load, instruction->destinations[0], instruction->destinations[0], ir_operand::create_con(instruction->sources[0].value * 8));
}

static void emit_with_possible_remaps(x86_pre_allocator_context* result, ir_operation* operation)
{
	ir_operand new_destinations[10];
	ir_operand new_sources[10];

	for (int i = 0; i < operation->destinations.count; ++i)
	{
		new_destinations[i] =  register_or_constant(result, operation->destinations[i]);
	}

	for (int i = 0; i < operation->sources.count; ++i)
	{
		new_sources[i] =  register_or_constant(result, operation->sources[i]);
	}

	ir_operation_block::emit_with(
		result->ir, 
		operation->instruction,
		new_destinations,		operation->destinations.count,
		new_sources,			operation->sources.count
	);
}

static void emit_as_is(x86_pre_allocator_context* result, ir_operation* operation)
{
	ir_operation_block* ir = result->ir;

	ir_operation_block::emit_with(
		ir, 
		operation->instruction,
		operation->destinations.data,	operation->destinations.count,
		operation->sources.data,		operation->sources.count
	);
}

static void emit_pre_allocation_instruction(x86_pre_allocator_context* pre_allocator_context, ir_operation* operation)
{
	ir_instructions working_instruction = (ir_instructions)operation->instruction;

	switch (working_instruction)
	{
		//Binary Operations
		case ir_add:
		case ir_subtract:
		case ir_bitwise_and:
		case ir_bitwise_or:
		case ir_bitwise_exclusive_or:
		case ir_multiply:
		{
			assert_operand_count(operation, 1, 2);
			assert_is_register(operation->destinations[0]);

			emit_d_n_f_d_n_m(pre_allocator_context, operation->instruction, operation->destinations[0], operation->sources[0], operation->sources[1]);
		};  break;

		case ir_multiply_hi_signed:
		case ir_multiply_hi_unsigned:
		case ir_divide_unsigned:
		case ir_divide_signed:
		{
			assert_operand_count(operation, 1, 2);
			assert_is_register(operation->destinations[0]);

			emit_big_multiply_divide(pre_allocator_context, operation->instruction, operation->destinations[0], operation->sources[0], operation->sources[1]);
		}; break;

		case ir_compare_equal:
		case ir_compare_greater_equal_signed:
		case ir_compare_greater_equal_unsigned:
		case ir_compare_greater_signed:
		case ir_compare_greater_unsigned:
		case ir_compare_less_equal_signed:
		case ir_compare_less_equal_unsigned:
		case ir_compare_less_signed:
		case ir_compare_less_unsigned:
		case ir_compare_not_equal:
		{
			assert_operand_count(operation, 1, 2);
			assert_is_register(operation->destinations[0]);

			emit_compare(pre_allocator_context, operation->instruction, operation->destinations[0], operation->sources[0], operation->sources[1]);

		}; break;

		case ir_shift_left:
		case ir_shift_right_signed:
		case ir_shift_right_unsigned:
		case ir_rotate_right:
		{
			assert_operand_count(operation, 1, 2);
			assert_is_register(operation->destinations[0]);

			emit_shift(pre_allocator_context, operation->instruction, operation->destinations[0], operation->sources[0], operation->sources[1]);

		}; break;

		//ABI
		case ir_close_and_return:
		{
			assert_operand_count(operation, 0, 1);

			emit_return(pre_allocator_context, operation->sources[0]);

		}; break;

		case ir_get_argument:
		{
			assert_operand_count(operation, 1, 1);
			assert_is_constant(operation->sources[0]);

			emit_argument_load(pre_allocator_context, operation);
		}; break;

		//Unary Operations
		case ir_move:
		{
			assert_operand_count(operation, 1, 1);
			assert_is_register(operation->destinations[0]);

			emit_move(pre_allocator_context, operation->destinations[0], operation->sources[0]);

		}; break;

		case ir_bitwise_not:
		case ir_incrament: 
		case ir_negate:
		{
			assert_operand_count(operation, 1, 1);
			assert_is_register(operation->destinations[0]);

			emit_d_f_d_n(pre_allocator_context, operation->instruction, operation->destinations[0], operation->sources[0]);

		}; break;

		case ir_sign_extend:
		{
			assert_operand_count(operation, 1, 1);
			assert_is_register(operation->destinations[0]);

			if (ir_operand::get_raw_size(&operation->destinations[0]) == ir_operand::get_raw_size(&operation->sources[0]))
			{
				emit_move(pre_allocator_context, operation->destinations[0], operation->sources[0]);
			}
			else
			{
				ir_operand source = register_or_constant(pre_allocator_context, &operation->sources[0]);

				ir_operation_block::emitds(pre_allocator_context->ir, ir_instructions::ir_sign_extend, operation->destinations[0], source);
			}

		}; break;

		//Misc
		case ir_no_operation:
		{
			//Do nothing lolESS
		}; break;

		case ir_mark_label:
		{
			emit_as_is(pre_allocator_context, operation);
			
			assert(operation->sources[0].value < UINT32_MAX);
		}; break;

		case ir_load:
		case ir_store:
		case ir_jump_if:
		{
			emit_as_is(pre_allocator_context, operation);
		}; break;

		case ir_assert_false:
		case ir_assert_true:
		{
			emit_with_possible_remaps(pre_allocator_context, operation);
		}; break;

		case ir_conditional_select:
		{
			assert_operand_count(operation, 1, 3);
			assert_is_register(operation->destinations[0]);

			emit_conditional_select(pre_allocator_context, operation->destinations[0], operation->sources[0], operation->sources[1], operation->sources[2]);
		}; break;

		case ir_double_shift_right:
		{
			assert_operand_count(operation, 1, 3);
			assert_is_register(operation->destinations[0]);

			emit_double_shift_right(pre_allocator_context, operation->destinations[0], operation->sources[0], operation->sources[1], operation->sources[2]);
		}; break;

		default: assert(false); throw 0;
	}

	for (int i = 0; i < operation->destinations.count; ++i)
	{
		ir_operand destination = operation->destinations[i];

		uint64_t raw_size = ir_operand::get_raw_size(&destination);

		if (raw_size == ir_operand_meta::int16 || raw_size == ir_operand_meta::int8)
		{
			ir_operand::set_raw_size(&destination, ir_operand_meta::int64);

			uint64_t mask = (1ULL << (8 << raw_size)) - 1;

			ir_operation_block::emitds(pre_allocator_context->ir, ir_instructions::ir_bitwise_and, destination, destination, ir_operand::create_con(mask, ir_operand_meta::int64));
		}
	}
}

void x86_pre_allocator_context::run_pass(x86_pre_allocator_context* pre_allocator_context, ir_operation_block* result_ir, ir_operation_block* source, cpu_information cpu_data, os_information os)
{
	assert(cpu_data == x86_64);

	ir_operation_block::clamp_operands(source, false);

	pre_allocator_context->allocator = result_ir->allocator;
	pre_allocator_context->scrap_index = 0;
	pre_allocator_context->ir = result_ir;
	pre_allocator_context->source_ir = source;

	pre_allocator_context->revisit_instructions = intrusive_linked_list<intrusive_linked_list_element<ir_operation>*>::create(pre_allocator_context->allocator, nullptr, nullptr);

	pre_allocator_context->ir->label_index = UINT32_MAX;

	for (auto i = source->operations->first; i != source->operations->last; i = i->next)
	{
		emit_pre_allocation_instruction(pre_allocator_context, &i->data);

		pre_allocator_context->scrap_index = 0;
	}

	ir_operation_block::clamp_operands(result_ir, false, pre_allocator_context->opernad_counts);

	uint64_t context_size = (pre_allocator_context->opernad_counts[0] * 8) + (pre_allocator_context->opernad_counts[1] * 16);

	context_size = get_context_size(context_size);

	ir_operation_block::emits(result_ir, ir_instructions::ir_open_context, ir_operand::create_con(context_size), result_ir->operations->first);

	for (auto i = pre_allocator_context->revisit_instructions->first; i != pre_allocator_context->revisit_instructions->last; i = i->next)
	{
		intrusive_linked_list_element<ir_operation>* working_element = i->data;

		if (working_element == nullptr)
			continue;

		switch (working_element->data.instruction)
		{
		case ir_get_argument:
			working_element->data.instruction = ir_load; 
			working_element->data.sources[1].value = context_size + 8;
			; break;
		case ir_close_and_return: 
			working_element->data.sources[1].value = context_size; break;
		default: throw 0;
		}
	}

	ir_operation_block::emits(result_ir, ir_instructions::ir_register_allocator_p_lock, RSP(ir_operand_meta::int64), result_ir->operations->first);
}
