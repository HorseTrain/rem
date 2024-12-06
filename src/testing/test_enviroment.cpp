#include "test_enviroment.h"
#include "ir_emulator.h"

void test_enviroment::create(test_enviroment* test_enviroment_context, abi working_abi)
{    
    test_enviroment_context->allocator = arena_allocator::create(1024);
    test_enviroment_context->working_abi = working_abi;
    
    ir_operation_block::create(&test_enviroment_context->ir, &test_enviroment_context->allocator);
    jit_context::create(&test_enviroment_context->jit, 1024 * 1024, test_enviroment_context->working_abi);
}

void test_enviroment::destroy(test_enviroment* test_enviroment_context)
{
    arena_allocator::destroy(&test_enviroment_context->allocator);
    jit_context::destroy(&test_enviroment_context->jit);
}

uint64_t test_enviroment::execute_emulator(test_enviroment* test_enviroment_context, void* arguments)
{
    ir_emulator emulator_context;
    ir_emulator::create(&emulator_context);

    uint64_t return_value = ir_emulator::execute(&emulator_context, test_enviroment_context->ir, (uint64_t*)arguments);

    ir_emulator::destroy(&emulator_context);

    return return_value;
}

uint64_t test_enviroment::execute_jit(test_enviroment* test_enviroment_context, void* arguments)
{
    void* code = jit_context::compile_code(&test_enviroment_context->jit, test_enviroment_context->ir);

    return jit_context::call_jitted_function(&test_enviroment_context->jit, code, (uint64_t*)arguments);
}