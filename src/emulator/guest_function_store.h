#ifndef GUEST_FUNCTION_STORE_H
#define GUEST_FUNCTION_STORE_H

#include <unordered_map>
#include <inttypes.h>
#include <mutex>

#include "translate_request_data.h"
#include "fast_function_table.h"

struct guest_function_store;
struct translate_request_data;

struct guest_function_store
{
    std::mutex                                      lock;
    std::unordered_map<uint64_t, guest_function>    functions;
    translate_guest_function                        translate_function_pointer;
    fast_function_table                             native_function_table;
    bool                                            use_flt;

    static guest_function                           get_or_translate_function(guest_function_store* context, uint64_t address, translate_request_data* process_context);
    static void                                     destroy(guest_function_store* to_destory);
};

#endif
