#include "test_cases.h"
#include "ir/ir.h"
#include "test_enviroment.h"
#include "tools/numbers.h"

ir_operand random_constant(uint64_t size)
{
    uint64_t value = create_random_number();

    return ir_operand::create_con(value, size);
}

ir_operand random_reg(uint64_t size, int max_registers)
{
    int reg = rand() % max_registers;

    return ir_operand::create_reg(reg, size);
}

static void emit_binary_operations(test_enviroment* test,abi abi_information, ir_instructions instruction, int iterations, int reg_max)
{
    ir_operation_block* ir = test->ir;

    for (int i = 0; i < iterations; ++i)
    {
        uint64_t working_size = int8 + (i % 4);
        
        ir_operand n;
        ir_operand m;
        ir_operand r = random_reg(working_size, reg_max);

        if (i & 1)
        {
            n = random_constant(working_size);
            m = random_constant(working_size);

            if (instruction == ir_divide_signed || instruction == ir_divide_unsigned)
            {
                m.value |= 1;
            }
        }
        else
        {
            n = random_reg(working_size, reg_max);
            m = random_reg(working_size, reg_max);

            if (instruction == ir_divide_signed || instruction == ir_divide_unsigned)
            {
                ir_operation_block::emitds(ir,ir_bitwise_or,m, m, ir_operand::create_con(1, working_size));
            }
        }

        ir_operation_block::emitds(ir,instruction,r, n, m);
    }
}

void test_all(abi abi_information)
{
    srand(0);

    test_enviroment enviroment;
    test_enviroment::create(&enviroment, abi_information);

    int register_max = 100;

    for (int i = 0; i < register_max; ++i)
    {
        ir_operation_block::emitds(enviroment.ir, ir_move, ir_operand::create_reg(i), ir_operand::create_con(create_random_number()));
    }

    emit_binary_operations(&enviroment,abi_information, ir_add, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_bitwise_and, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_bitwise_exclusive_or, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_bitwise_or, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_equal, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_equal_signed, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_equal_unsigned, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_signed, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_unsigned, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_equal_signed, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_equal_unsigned, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_signed, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_unsigned, 1, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_not_equal, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_divide_signed, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_divide_unsigned, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_signed, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_signed, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_unsigned, 1000, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_subtract, 1000, register_max);

    ir_operation_block::emits(enviroment.ir, ir_close_and_return, random_reg(int64,register_max));

    uint64_t jit_result = test_enviroment::execute_jit(&enviroment, nullptr);
    uint64_t emulator_result = test_enviroment::execute_emulator(&enviroment, nullptr);

    assert(emulator_result == jit_result);

    test_enviroment::destroy(&enviroment);
}