#ifndef JIT_CONTEXT_H
#define JIT_CONTEXT_H

#include "growing_jit_cache.h"
#include "abi_information.h"

struct ir_operation_block;

struct jit_context
{
    growing_jit_cache   jit_cache;

    static void create(jit_context* result,uint64_t allocation_size, abi abi_information);
    static void destroy(jit_context* to_destroy);
    static void create_from_host(jit_context* result, uint64_t allocation_size);
    static uint64_t call_jitted_function(jit_context* context, void* function, uint64_t* arguments);
    static void* append_to_jit_cache(jit_context* context, void* source_function, uint64_t function_size);
    static void* compile_code(jit_context* context, ir_operation_block* ir_operation_block_context);
};

#endif