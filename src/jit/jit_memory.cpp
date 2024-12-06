#include <assert.h>
#include <string.h>

#include "jit_memory.h"

#define USE_MEMPROTECT (defined __linux__) || (defined __APPLE__)
#define USE_VIRTUAL_PROTECT (defined _WIN32)

#if USE_MEMPROTECT
#include "sys/mman.h"

static bool allocate_executable_memory(void** memory, uint64_t size)
{
    void* result = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    *memory = result;

    return result != nullptr;
}

static void unmark_memory_executable(void* memory, uint64_t size)
{
    munmap(memory, size);
}

#elif USE_VIRTUAL_PROTECT
#include "windows.h"

static bool allocate_executable_memory(void** memory, uint64_t size)
{
    void* result = VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE);

    DWORD dummy;
    bool success = VirtualProtect(result, size, PAGE_EXECUTE_READWRITE, &dummy) != 0;

    *memory = result;

    return success;
}

static void unmark_memory_executable(void* memory, uint64_t size)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

//TODO:

#endif

static uint64_t align_64_kb(uint64_t source)
{
    uint64_t mask = ~63ULL;

    return (source & mask) + 64;
}

bool jit_memory::create(jit_memory** result, uint64_t allocation_size, abi host_abi)
{
    jit_memory* working_result = new jit_memory();

    allocation_size = align_64_kb(allocation_size);

    working_result->host_abi = host_abi;
    working_result->memory_block_size = allocation_size;

    *result = working_result;

    return allocate_executable_memory(&working_result->raw_memory_block, allocation_size);
}

void jit_memory::destroy(jit_memory* to_destroy)
{
    unmark_memory_executable(to_destroy->raw_memory_block, to_destroy->memory_block_size);

    delete to_destroy;
}

void* jit_memory::coppy_over(jit_memory* jit_memory_context,uint64_t result_offset, void* source, uint64_t size)
{
    char* result_location = (char*)jit_memory_context->raw_memory_block + result_offset;
    
    memcpy(result_location, source, size);

    return result_location;
}