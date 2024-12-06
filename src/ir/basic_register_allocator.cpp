#include "basic_register_allocator.h"

static register_allocator_module* create_allocator_module(basic_register_allocator_context* main_unit, arena_allocator* allocator, int host_count, int guest_count, uint64_t guest_type)
{
	register_allocator_module* result = arena_allocator::allocate_struct< register_allocator_module>(allocator);

	result->allocator_unit = main_unit;

	result->host_register_count = host_count;
	result->host_registers = arena_allocator::allocate_struct<host_register>(allocator, host_count);

	for (int i = 0; i < host_count; ++i)
	{
		host_register* working_guest_register = &result->host_registers[i];

		working_guest_register->host_index = i;
		working_guest_register->guest_offset = -1;

		working_guest_register->lock_data = lock_mode::unlocked;

		working_guest_register->working_mode = register_mode::none;
	}

	result->guest_count = guest_count;
	result->guest_type = guest_type;

	return result;
}

static register_allocator_module* get_module(basic_register_allocator_context* context, ir_operand test)
{
	if (ir_operand::is_vector(&test))
	{
		return context->vec_allocator;
	}

	return context->gp_allocator;
}

static void init_p_lock(basic_register_allocator_context* context, ir_operand reg)
{
	register_allocator_module* working_module = get_module(context, reg);

	assert(ir_operand::is_hardware(&reg));

	host_register* host = &working_module->host_registers[reg.value];

	if (host_register::is_locked(host))
	{
		assert(false);

		throw 0;
	}
	else
	{
		register_allocator_module::emit_host_unload(working_module, host->host_index);

		assert(host->guest_offset == -1);

		host->lock_data = lock_mode::force_lock;
	}
}

static void init_p_unlock(basic_register_allocator_context* context, ir_operand reg)
{
	register_allocator_module* working_module = get_module(context, reg);

	assert(ir_operand::is_hardware(&reg));

	host_register* host = &working_module->host_registers[reg.value];

	if (host->lock_data & lock_mode::force_lock)
	{
		host_register::unset_lock_bit(host, lock_mode::force_lock);
	}
	else
	{
		assert(false);

		throw 0;
	}
}

static int get_guest_offset(register_allocator_module* module, int guest_index)
{
	int byte_count = 1 << module->guest_type;

	return module->stack_offset + (guest_index * byte_count);
}

static int request_guest_register(register_allocator_module* module, int guest_register, register_mode mode)
{
	guest_register = get_guest_offset(module, guest_register);

	for (int i = 0; i < module->host_register_count; ++i)
	{
		host_register* working_host_register = &module->host_registers[i];

		if (working_host_register->guest_offset == guest_register)
		{
			working_host_register->working_mode = (register_mode)(working_host_register->working_mode | mode);
			working_host_register->lock_data = lock_mode::basic_lock;

			working_host_register->hits++;

			return i;
		}
	}

	for (int i = 0; i < module->host_register_count; ++i)
	{
		host_register* working_host_register = &module->host_registers[i];

		if (host_register::is_free(working_host_register))
		{
			register_allocator_module::emit_host_load(module, i, guest_register, mode);

			return i;
		}
	}

	//Is there a faster way than just a linier
	//search?
	int lowest_hit			= INT32_MAX;
	int lowest_hit_index	= -1;

	for (int i = 0; i < module->host_register_count; ++i)
	{
		host_register* working_host_register = &module->host_registers[i];

		if (host_register::is_locked(working_host_register))
			continue;

		if (working_host_register->hits < lowest_hit)
		{
			lowest_hit = working_host_register->hits;
			lowest_hit_index = i;

			if (working_host_register->hits == 0)
			{
				break;
			}
		}
	}

	if (lowest_hit_index == -1)
	{
		assert(false);

		throw 0;
	}

	register_allocator_module::emit_host_load(module, lowest_hit_index, guest_register, mode);

	return lowest_hit_index;
}

static void allocate_register(basic_register_allocator_context* context, ir_operand* result, ir_operand* source, register_mode mode)
{
	if (ir_operand::is_constant(source))
	{
		*result = *source;
	}
	else if (ir_operand::is_hardware(source))
	{
		*result = ir_operand::create_reg(source->value, source->meta_data);
	}
	else
	{
		register_allocator_module* working_module = get_module(context, *source);

		int host_register = request_guest_register(working_module, source->value, mode);

		*result = ir_operand::create_reg(host_register, source->meta_data);
	}
}

static void allocate_registers(basic_register_allocator_context* context, ir_operand* result, int result_max, ir_operand* sources, int source_count, register_mode mode)
{
	assert(source_count <= result_max);

	for (int i = 0; i < source_count; ++i)
	{
		allocate_register(context, &result[i], &sources[i], mode);
	}
}

static void unlock_host_basic(host_register* host)
{
	host_register::unset_lock_bit(host, lock_mode::basic_lock);
}

static void unlock_all_basic(register_allocator_module* context)
{
	for (int i = 0; i < context->host_register_count; ++i)
	{
		unlock_host_basic(&context->host_registers[i]);
	}
}

static void unload_all(register_allocator_module* context)
{
	for (int i = 0; i < context->host_register_count; ++i)
	{
		register_allocator_module::emit_host_unload(context, i);
	}
}

static void unlock_all_basic(basic_register_allocator_context* context)
{
	unlock_all_basic(context->gp_allocator);
	unlock_all_basic(context->vec_allocator);
}

static void unload_all(basic_register_allocator_context* context)
{
	unload_all(context->gp_allocator);
	unload_all(context->vec_allocator);
}

void host_register::set_lock_bit(host_register* guest, lock_mode mode)
{
	guest->lock_data = (lock_mode)(guest->lock_data | mode);
}

void host_register::unset_lock_bit(host_register* guest, lock_mode mode)
{
	guest->lock_data = (lock_mode)(guest->lock_data & ~mode);
}

bool host_register::is_locked(host_register* guest)
{
	return guest->lock_data != lock_mode::unlocked;
}

bool host_register::is_loaded(host_register* host)
{
	return host->guest_offset != -1;
}

bool host_register::is_free(host_register* host)
{
	return !is_locked(host) && !is_loaded(host);
}

void register_allocator_module::emit_host_load(register_allocator_module* module, int host_index, int guest_offset, register_mode working_mode)
{
	ir_operation_block* result_ir = module->allocator_unit->result_ir;
	host_register* working_host = &module->host_registers[host_index];

	emit_host_unload(module, host_index);

	working_host->guest_offset = guest_offset;
	working_host->working_mode = working_mode;

	host_register::set_lock_bit(working_host, lock_mode::basic_lock);

	if (working_mode & register_mode::read)
	{
		ir_operand offset = ir_operand::create_con(working_host->guest_offset);
		ir_operand result_location = ir_operand::create_reg(working_host->host_index, module->guest_type);

		ir_operation_block::emitds(result_ir, ir_instructions::ir_load, result_location, module->allocator_unit->context_register, offset);
	}
}

void register_allocator_module::emit_host_unload(register_allocator_module* module, int host_index)
{
	ir_operation_block* result_ir = module->allocator_unit->result_ir;
	host_register* working_host = &module->host_registers[host_index];

	if (host_register::is_locked(working_host) && host_register::is_loaded(working_host))
	{
		assert(false);

		throw 0;
	}

	if (!host_register::is_loaded(working_host))
	{
		return;
	}

	int type_byte_count = 8 << module->guest_type;
	
	if (working_host->working_mode & register_mode::write)
	{
		ir_operand offset = ir_operand::create_con(working_host->guest_offset);
		ir_operand to_store = ir_operand::create_reg(working_host->host_index, module->guest_type);

		ir_operation_block::emits(result_ir, ir_instructions::ir_store, module->allocator_unit->context_register, offset, to_store);
	}

	working_host->guest_offset = -1;
	working_host->lock_data = lock_mode::unlocked;
	working_host->working_mode = register_mode::none;
	working_host->hits = 0;
}

void basic_register_allocator_context::run_pass(basic_register_allocator_context* result_register_allocator, ir_operation_block* result_ir, ir_operation_block* pre_allocated_code, int gp_count, guest_data gp_data, int vec_count, guest_data vec_data, ir_operand context_register)
{
	arena_allocator* allocator = result_ir->allocator;

	result_register_allocator->gp_allocator = create_allocator_module(result_register_allocator, allocator, gp_count, gp_data.guest_count, gp_data.guest_type);
	result_register_allocator->vec_allocator = create_allocator_module(result_register_allocator, allocator, vec_count, vec_data.guest_count, vec_data.guest_type);

	result_register_allocator->gp_allocator->stack_offset = 0;
	result_register_allocator->vec_allocator->stack_offset = (gp_data.guest_count * 8);

	result_register_allocator->context_register = context_register;

	result_register_allocator->result_ir = result_ir;

	for (auto i = pre_allocated_code->operations->first; i != pre_allocated_code->operations->last; i = i->next)
	{
		ir_operation working_operation = i->data;

		unlock_all_basic(result_register_allocator);

		switch (working_operation.instruction)
		{
		case ir_instructions::ir_register_allocator_p_lock:
		{
			init_p_lock(result_register_allocator, working_operation.sources[0]);

		}; break;

		case ir_instructions::ir_register_allocator_p_unlock:
		{
			init_p_unlock(result_register_allocator, working_operation.sources[0]);
		}; break;

		default:
		{
			const int stack_max = 10;

			ir_operand new_sources[stack_max];
			ir_operand new_destinations[stack_max];

			uint64_t instruction = working_operation.instruction;

			allocate_registers(result_register_allocator, new_sources, stack_max, working_operation.sources.data, working_operation.sources.count, register_mode::read);
			allocate_registers(result_register_allocator, new_destinations, stack_max, working_operation.destinations.data, working_operation.destinations.count, register_mode::write);

			if (ir_operation_block::is_label(instruction))
			{
				unlock_all_basic(result_register_allocator);

				unload_all(result_register_allocator);
			}

			ir_operation_block::emit_with(result_ir, instruction, new_destinations, working_operation.destinations.count, new_sources, working_operation.sources.count);

		}; break;
		}
	}
}