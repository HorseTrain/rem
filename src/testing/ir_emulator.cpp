#include "ir_emulator.h"
#include "ir/checks.h"
#include "tools/numbers.h"
#include "tools/big_number.h"

#define GENERIC_TEMPLATE tenplate<typename T>

template <typename T>
static void sign_extend_bottom(big_number<T, 2>* working)
{
    int top_bit = (sizeof(T) * 8) - 1;

    bool bit = working->buffer[0] >> top_bit;

    if (bit)
    {
        working->buffer[1] = -1;
    }
}

template <typename T>
static void multiply_hi(T* top, T* bottom, T x, T y, bool is_signed)
{
    big_number<T, 2> wx = x;
    big_number<T, 2> wy = y;

    if (is_signed)
    {
        sign_extend_bottom(&wx);
        sign_extend_bottom(&wy);
    }

    big_number<T, 2> r = wx * wy;

    *top = r.buffer[1];
    *bottom = r.buffer[0];
}

static void init_ir_emulator_context(ir_emulator* ir_emulator_context, ir_operation_block* to_execute, uint64_t* arguments)
{
    ir_emulator_context->working_registers = std::unordered_map<uint64_t, uint64_t>();
    ir_emulator_context->arguments = arguments;
}

static void advance(ir_emulator* ir_emulator_context)
{
    ir_emulator_context->working_instruction_element = ir_emulator_context->working_instruction_element->next;
}

static uint64_t get_mask_from_size(uint64_t size)
{
    if (size == int64)
        return -1;

    assert(size != int128);

    uint64_t mask = (1ULL << (8 << size)) - 1;

    return mask;
}

static uint64_t sign_extend_from_size(uint64_t source, uint64_t size)
{
    if (size == int64)
        return source;

    assert(size != int128);

    int bit = 63 - ((8 << size) - 1);

    return ((int64_t)source << bit) >> bit;
}

static uint64_t zero_extend_from_size(uint64_t source, uint64_t size)
{
    return source & get_mask_from_size(size);
}

static uint64_t* get_destination(ir_emulator* ir_emulator_context, ir_operand working_operand)
{
    assert_is_register(working_operand);

    if (ir_emulator_context->working_registers.find(working_operand.value) == ir_emulator_context->working_registers.end())
    {
        ir_emulator_context->working_registers[working_operand.value] = 0;
    }

    return &ir_emulator_context->working_registers[working_operand.value];
}

static uint64_t get_source(ir_emulator* ir_emulator_context, ir_operand working_operand)
{
    uint64_t result;

    if (ir_operand::is_constant(&working_operand))
    {
        result = working_operand.value;
    }
    else
    {
        if (ir_emulator_context->working_registers.find(working_operand.value) == ir_emulator_context->working_registers.end())
        {
            ir_emulator_context->working_registers[working_operand.value] = 0;
        }

        result = ir_emulator_context->working_registers[working_operand.value];
    }

    return result & get_mask_from_size(ir_operand::get_raw_size(&working_operand));
}

static void execute_unary_operation(ir_emulator* ir_emulator_context)
{
    ir_operation working_instruction = ir_emulator_context->working_instruction_element->data;

    assert_operand_count(&working_instruction, 1, 1);
    assert_registers_same_type(&working_instruction);

    ir_operand* destinations = working_instruction.destinations.data;
    ir_operand* sources = working_instruction.sources.data;

    assert_is_register(destinations[0]);

    uint64_t working_size = ir_operand::get_raw_size(&destinations[0]);

    uint64_t x = get_source(ir_emulator_context, sources[0]);

    uint64_t result;

    switch (working_instruction.instruction)
    {
        case ir_move: result = x; break;
    default:
        break;
    }
    
    *get_destination(ir_emulator_context, destinations[0]) = zero_extend_from_size(result, working_size);

    advance(ir_emulator_context);
}

static void execute_operation_binary(ir_emulator* ir_emulator_context)
{
    ir_operation working_instruction = ir_emulator_context->working_instruction_element->data;

    assert_operand_count(&working_instruction, 1, 2);
    assert_registers_same_type(&working_instruction);

    ir_operand* destinations = working_instruction.destinations.data;
    ir_operand* sources = working_instruction.sources.data;

    assert_is_register(destinations[0]);

    uint64_t working_size = ir_operand::get_raw_size(&destinations[0]);

    uint64_t x = get_source(ir_emulator_context, sources[0]);
    uint64_t y = get_source(ir_emulator_context, sources[1]);

    uint64_t result;

    switch (working_instruction.instruction)
    {
        case ir_add:                            result = x + y; break;
        case ir_bitwise_and:                    result = x & y; break;
        case ir_bitwise_exclusive_or:           result = x ^ y; break;
        case ir_bitwise_or:                     result = x | y; break;
        case ir_compare_equal:                  result = x == y; break;
        case ir_compare_greater_equal_signed:   result = sign_extend_from_size(x, working_size) >= sign_extend_from_size(x, working_size); break;
        case ir_compare_greater_equal_unsigned: result = x >= x; break;
        case ir_compare_greater_signed:         result = sign_extend_from_size(x, working_size) > sign_extend_from_size(x, working_size); break;
        case ir_compare_greater_unsigned:       result = x >  x; break;
        case ir_compare_less_equal_signed:      result = sign_extend_from_size(x, working_size) <= sign_extend_from_size(x, working_size); break;
        case ir_compare_less_equal_unsigned:    result = x <= x; break;
        case ir_compare_less_signed:            result = sign_extend_from_size(x, working_size) < sign_extend_from_size(x, working_size); break;
        case ir_compare_less_unsigned:          result = x <  x; break;
        case ir_compare_not_equal:              result = x != y; break;
        case ir_divide_unsigned:                result = x / y; break;
        case ir_multiply:                       result = x * y; break;
        case ir_subtract:                       result = x - y; break;


        case ir_multiply_hi_unsigned:   
        case ir_multiply_hi_signed:
        {
            uint64_t top;
            uint64_t bottom;

            bool is_signed = working_instruction.instruction == ir_multiply_hi_signed;

            switch (working_size)
            {
            case int8:  multiply_hi<uint8_t>((uint8_t*)&top, (uint8_t*)&bottom, x, y, is_signed);       break;
            case int16: multiply_hi<uint16_t>((uint16_t*)&top, (uint16_t*)&bottom, x, y, is_signed);    break;
            case int32: multiply_hi<uint32_t>((uint32_t*)&top, (uint32_t*)&bottom, x, y, is_signed);    break;
            case int64: multiply_hi<uint64_t>((uint64_t*)&top, (uint64_t*)&bottom, x, y, is_signed);    break;
            
            default:
                break;
            }

            result = top;
        }; break;
        
        case ir_divide_signed:
        {
            int64_t sx = sign_extend_from_size(x, working_size);
            int64_t sy = sign_extend_from_size(y, working_size);

            result = sx / sy;
        }; break;

    default:
        break;
    }
    
    *get_destination(ir_emulator_context, destinations[0]) = zero_extend_from_size(result, working_size);

    advance(ir_emulator_context);
}

void ir_emulator::create(ir_emulator* ir_emulator_context)
{
    ir_emulator_context->arguments = nullptr;
}

void ir_emulator::destroy(ir_emulator* to_destroy)
{
    
}

uint64_t ir_emulator::execute(ir_emulator* ir_emulator_context, ir_operation_block* to_execute, uint64_t* arguments)
{
    init_ir_emulator_context(ir_emulator_context,to_execute, arguments);

    ir_emulator_context->working_instruction_element = to_execute->operations->first;

    while (1)
    {
        if (ir_emulator_context->working_instruction_element == nullptr)
        {
            //This should NEVER happen

            assert(false);

            throw 0;

            break;
        }

        ir_operation working_instruction = ir_emulator_context->working_instruction_element->data;

        switch (working_instruction.instruction)
        {
            case ir_add:
            case ir_bitwise_and:
            case ir_bitwise_exclusive_or:
            case ir_bitwise_or:
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
            case ir_divide_signed:
            case ir_divide_unsigned:
            case ir_multiply:
            case ir_multiply_hi_signed:
            case ir_multiply_hi_unsigned:
            case ir_subtract:
            {
                execute_operation_binary(ir_emulator_context);
            }; break;

            case ir_move:
            case ir_negate:
            case ir_bitwise_not:
            {
                execute_unary_operation(ir_emulator_context);
            }; break;;

            case ir_no_operation:
            {
                advance(ir_emulator_context);
            }; break;

            case ir_close_and_return:
            {
                assert_operand_count(&working_instruction, 0, 1);

                uint64_t value_to_return = get_source(ir_emulator_context, working_instruction.sources[0]);

                return value_to_return;
            }; break;

            default:
            {
                assert(false);

                throw 0;

                break;
            };
        }
    }
}