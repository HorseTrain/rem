#ifndef JIT_MEMORY_H
#define JIT_MEMORY_H

#include <inttypes.h>

#include "abi_information.h"

struct jit_memory
{
    abi             host_abi;

    void*           raw_memory_block;
    uint64_t        memory_block_size;

    static bool     create(jit_memory** result, uint64_t allocation_size,abi host_abi);
    static void     destroy(jit_memory* to_destroy);
    static void*    coppy_over(jit_memory* jit_memory_context,uint64_t result_offset, void* source, uint64_t size);
};

#endif