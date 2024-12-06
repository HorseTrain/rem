#include <assert.h>

#include "jit_context.h"
#include "jit_memory.h"

#include "assembly/x86/x86_pipeline.h"
#include "assembly/x86/x86_assembler.h"

typedef uint64_t (*abi_caller_function)(void*, uint64_t*);

static void create_x86_caller(jit_context* result)
{
    uint64_t max_space = 400;

    void* caller_code_space = growing_jit_cache::allocate(&result->jit_cache, max_space);

    uint64_t code_size;
    assemble_x86_abi_caller_code(caller_code_space, &code_size, result->jit_cache.memory->host_abi);

    if (code_size > max_space)
    {
        assert(false);
        throw 0;
    }
}

void jit_context::create(jit_context* result,uint64_t allocation_size, abi abi_information)
{
    jit_memory* jit_memory_context;

    jit_memory::create(&jit_memory_context, allocation_size, abi_information); 
    growing_jit_cache::create(&result->jit_cache, jit_memory_context);

    switch (abi_information.cpu)
    {
        case x86_64:
        {
            create_x86_caller(result);
        }; break;

        default:
        {
            assert(false);
            throw 0;
        }; break;
    }
}

void jit_context::destroy(jit_context* to_destroy)
{
    jit_memory::destroy(to_destroy->jit_cache.memory);
}

void jit_context::create_from_host(jit_context* result, uint64_t allocation_size)
{
    create(result, allocation_size, get_abi());
}

uint64_t jit_context::call_jitted_function(jit_context* context, void* function, uint64_t* arguments)
{
    abi_caller_function abi_function = (abi_caller_function)context->jit_cache.memory->raw_memory_block;

    return abi_function(function, arguments);
}

void* jit_context::append_to_jit_cache(jit_context* context, void* source_function, uint64_t function_size)
{
    return growing_jit_cache::append_code(&context->jit_cache, source_function, function_size);
}

void* jit_context::compile_code(jit_context* context, ir_operation_block* ir_operation_block_context)
{
    void* code_buffer;
    uint64_t code_size;

    abi working_abi = context->jit_cache.memory->host_abi;

    switch (context->jit_cache.memory->host_abi.cpu)
    {
        case x86_64:
        {
            assemble_x86_64_pipeline(&code_buffer, &code_size, ir_operation_block_context, false, working_abi);
        }; break;

        default: throw 0;
    }

    return jit_context::append_to_jit_cache(context, code_buffer, code_size);
}