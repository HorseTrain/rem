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
        uint64_t working_size = i % 4;
        
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

void test_all(abi abi_information, int iteration)
{
    srand(iteration);

    test_enviroment enviroment;
    test_enviroment::create(&enviroment, abi_information);

    int register_max = 20;

    for (int i = 0; i < register_max; ++i)
    {
        ir_operation_block::emitds(enviroment.ir, ir_move, ir_operand::create_reg(i), ir_operand::create_con(create_random_number()));
    }

    int count = 20;
    
    emit_binary_operations(&enviroment,abi_information, ir_add, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_bitwise_and, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_bitwise_exclusive_or, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_bitwise_or, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_equal, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_divide_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_divide_unsigned, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_unsigned, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_subtract, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_equal_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_equal_unsigned, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_greater_unsigned, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_equal_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_equal_unsigned, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_less_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_equal, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_compare_not_equal, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_unsigned, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_multiply_hi_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_divide_signed, count, register_max);
    emit_binary_operations(&enviroment,abi_information, ir_divide_unsigned, count, register_max);

    for (int i = 0; i < register_max; ++i)
    {
        ir_operation_block::emitds(enviroment.ir, ir_bitwise_exclusive_or, ir_operand::create_reg(0, int64), ir_operand::create_reg(0, int64), ir_operand::create_reg(i, int64));
    }

    ir_operation_block::emits(enviroment.ir, ir_close_and_return, ir_operand::create_reg(0, int64));

    uint64_t jit_result = test_enviroment::execute_jit(&enviroment, nullptr);
    uint64_t emulator_result = test_enviroment::execute_emulator(&enviroment, nullptr);

    if (emulator_result != jit_result)
    {
        ir_operation_block::log(enviroment.ir);
    }

    std::cout << jit_result << " " << emulator_result << std::endl;

    assert(emulator_result == jit_result);

    test_enviroment::destroy(&enviroment);
}