#include "test_cases.h"
#include "ir/ir.h"
#include "test_enviroment.h"
#include "tools/numbers.h"
#include "tools/bit_tools.h"
#include <vector>

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

static void emit_operation(test_enviroment* test,abi abi_information, ir_instructions instruction, int iterations, int destination_count, int source_count,int reg_max)
{
    for (int size = 0; size < iterations; ++size)
    {
        std::vector<ir_operand> sources;
        std::vector<ir_operand> destinations;

        uint64_t working_size = size % 4;
        
        for (int i = 0; i < destination_count; ++i)
        {
            destinations.push_back(random_reg(working_size, reg_max));
        }

        for (int i = 0; i < source_count; ++i)
        {
            ir_operand src = rand() & 1 ? random_reg(working_size, reg_max) : random_constant(working_size);

            sources.push_back(src);
        }

        switch (instruction)
        {
            case ir_divide_signed:
            case ir_divide_unsigned:
            {
                if (ir_operand::is_constant(&sources[1]))
                {
                    sources[1].value &= 0b11ULL;
                    sources[1].value |= 1;
                }
                else
                {
                    ir_operation_block::emitds(test->ir, ir_bitwise_and, sources[1], sources[1], ir_operand::create_con(~0b11ULL, sources[1].meta_data));
                    ir_operation_block::emitds(test->ir, ir_bitwise_or, sources[1], sources[1], ir_operand::create_con(1, sources[1].meta_data));
                }
            }; break;

            case ir_shift_left:
            case ir_shift_right_signed:
            case ir_shift_right_unsigned:
            case ir_rotate_right:
            {
                int bit_count = 8 << sources[1].meta_data;
                uint64_t mask = bit_count - 1;

                if (ir_operand::is_constant(&sources[1]))
                {
                    sources[1].value &= mask;
                }
                else
                {
                    ir_operation_block::emitds(test->ir, ir_bitwise_and, sources[1], sources[1], ir_operand::create_con(mask, sources[1].meta_data));
                }
            }; break;

            case ir_double_shift_right:
            {
                int bit_count = 8 << ir_operand::get_raw_size(&sources[2]);
                uint64_t mask = bit_count - 1;

                if (ir_operand::is_constant(&sources[2]))
                {
                    sources[2].value &= mask;
                    sources[2].value |= 1;
                }
                else
                {
                    ir_operation_block::emitds(test->ir, ir_bitwise_and, sources[2], sources[2], ir_operand::create_con(mask, sources[2].meta_data));
                    ir_operation_block::emitds(test->ir, ir_bitwise_or, sources[2], sources[2], ir_operand::create_con(1, sources[2].meta_data));
                }
            }; break;
        }

        ir_operation_block::emit_with(test->ir, instruction, destinations.data(), destination_count, sources.data(), source_count);
    }
}

void test_all(abi abi_information, int iteration)
{
    srand(iteration);

    test_enviroment enviroment;
    test_enviroment::create(&enviroment, abi_information);

    int register_max = 6;

    for (int i = 0; i < register_max; ++i)
    {
        ir_operation_block::emitds(enviroment.ir, ir_move, ir_operand::create_reg(i), ir_operand::create_con(create_random_number()));
    }

    int count = 100;
    int iterations = 100;

    {
        ir_operand small_label = ir_operation_block::create_label(enviroment.ir);

        ir_operand small_iterator = ir_operand::create_reg(register_max);
        ir_operation_block::emitds(enviroment.ir, ir_move, small_iterator,ir_operand::create_con(iterations));

        ir_operation_block::mark_label(enviroment.ir,small_label);
        ir_operation_block::emitds(enviroment.ir, ir_subtract, small_iterator,small_iterator,ir_operand::create_con(1));

        for (int ins = ir_binary_start + 1; ins < ir_binary_end; ++ins)
        {
            emit_operation(&enviroment,abi_information, (ir_instructions)ins,count, 1, 2,register_max);
        }

        for (int ins = ir_unary_start + 1; ins < ir_unary_end; ++ins)
        {
            emit_operation(&enviroment,abi_information, (ir_instructions)ins,count, 1, 1, register_max);
        }

        for (int ins = ir_ternary_begin + 1; ins < ir_ternary_end; ++ins)
        {
            emit_operation(&enviroment,abi_information, (ir_instructions)ins,count, 1, 3, register_max);
        }

        ir_operation_block::jump_if(enviroment.ir, small_label, small_iterator);
    }

    ir_operation_block::emits(enviroment.ir, ir_close_and_return, random_reg(int64, register_max));

    uint64_t jit_result = test_enviroment::execute_jit(&enviroment, nullptr);
    uint64_t emulator_result = test_enviroment::execute_emulator(&enviroment, nullptr);

    std::cout << jit_result << " " << emulator_result << std::endl;

    assert(emulator_result == jit_result);

    test_enviroment::destroy(&enviroment);
}