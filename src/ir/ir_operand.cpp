#include "ir.h"

bool ir_operand::is_register(ir_operand* test)
{
	return !is_constant(test);
}

bool ir_operand::is_constant(ir_operand* test)
{
	return test->meta_data & ir_operand_meta::is_constant;
}

bool ir_operand::is_hardware(ir_operand* test)
{
	if (is_constant(test))
	{
		return false;
	}

	return test->meta_data & ir_operand_meta::is_hardware_register;
}

bool ir_operand::is_vector(ir_operand* test)
{
	uint64_t size_test = test->meta_data & UINT32_MAX;

	return size_test >= ir_operand_meta::int128;
}

uint64_t ir_operand::get_masked_constant(ir_operand* test)
{
	assert(is_constant(test));

	uint64_t bit_count = 8 << get_raw_size(test);

	if (bit_count == 64)
		return test->value;

	return test->value & ((1ULL << bit_count) - 1);
}

uint64_t ir_operand::get_raw_size(ir_operand* test)
{
	return test->meta_data & UINT32_MAX;
}

void ir_operand::set_raw_size(ir_operand* test, uint64_t new_size)
{
	test->meta_data = (test->meta_data & ~(uint64_t)UINT32_MAX) | (new_size & UINT32_MAX);
}

uint64_t ir_operand::get_meta(ir_operand* test)
{
	return test->meta_data & ~(uint64_t)UINT32_MAX;
}

ir_operand ir_operand::create_con(uint64_t value, uint64_t size)
{
	ir_operand result;

	result.value = value;
	result.meta_data = (size & UINT32_MAX) | ir_operand_meta::is_constant;

	return result;
}

ir_operand ir_operand::create_reg(uint64_t value, uint64_t size)
{
	ir_operand result;

	result.value = value;
	result.meta_data = (size & UINT32_MAX);

	return result;
}

ir_operand ir_operand::create_hardware_reg(uint64_t value, uint64_t size)
{
	ir_operand result;

	result.value = value;
	result.meta_data = (size & UINT32_MAX) | ir_operand_meta::is_hardware_register;

	return result;
}

ir_operand ir_operand::copy_new_raw_size(ir_operand source, uint64_t new_size)
{
	source.meta_data = (new_size & UINT32_MAX) | (source.meta_data & ~(uint64_t)UINT32_MAX);

	return source;
}

bool ir_operand::are_equal(ir_operand left, ir_operand right)
{
	bool type_equal = left.meta_data == right.meta_data;
	bool value_equal = left.value == right.value;

	return type_equal && value_equal;
}

bool ir_operand::same_size(ir_operand left, ir_operand right)
{
	return ir_operand::get_raw_size(&left) == ir_operand::get_raw_size(&right);
}