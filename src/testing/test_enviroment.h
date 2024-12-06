#ifndef TEST_ENVIROMENT_H
#define TEST_ENVIROMENT_H

#include "ir/ir.h"
#include "jit/jit_context.h"
#include "abi_information.h"

struct test_enviroment
{
    arena_allocator     allocator;
    ir_operation_block* ir;
    jit_context         jit;
    abi                 working_abi;

    static void         create(test_enviroment* test_enviroment_context, abi working_abi);
    static void         destroy(test_enviroment* test_enviroment_context);

    static uint64_t     execute_emulator(test_enviroment* test_enviroment_context, void* arguments);
    static uint64_t     execute_jit(test_enviroment* test_enviroment_context, void* arguments);
};

#endif