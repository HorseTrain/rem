#ifndef AARCH64_PROCESS_H
#define AARCH64_PROCESS_H

#include "emulator/guest_memory.h"
#include "emulator/guest_function_store.h"
#include "jit/jit_context.h"

#include "aarch64/aarch64_context_offsets.h"
#include "emulator/fixed_length_decoder.h"

struct guest_process
{
    guest_memory                    guest_memory_context;
    jit_context*                    host_jit_context;
    guest_function_store            guest_functions;
    fixed_length_decoder<uint32_t>  decoder;

    aarch64_context_offsets         guest_context_offset_data;

    void*                           svc_function;
    bool                            debug_mode;

    static void                     create(guest_process* result, guest_memory guest_memory_context, jit_context* host_jit_context, aarch64_context_offsets arm_guest_data);
    static uint64_t                 jit_function(guest_process* process, uint64_t guest_function, void* arm_context);
    static uint64_t                 interperate_function(guest_process* process, uint64_t guest_function, void* arm_context);

    static guest_function           translate_function(translate_request_data* data);
};

#endif