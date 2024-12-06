#ifndef GROWING_JIT_CACHE_H
#define GROWING_JIT_CACHE_H

#include <inttypes.h>

#include "abi_information.h"

struct jit_memory;

struct growing_jit_cache
{
    jit_memory*     memory;
    uint64_t        top;

    static void     create(growing_jit_cache* result, jit_memory* memory);
    static void*    append_code(growing_jit_cache* jit_cache, void* code, uint64_t size);
    static void*    allocate(growing_jit_cache* jit_cache, uint64_t size);
};

#endif