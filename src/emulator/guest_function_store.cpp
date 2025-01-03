#include "guest_function_store.h"
#include "ir/ir.h"

guest_function guest_function_store::get_or_translate_function(guest_function_store* context, uint64_t address, translate_request_data* process_context)
{
    context->lock.lock();

    if (context->functions.find(address) == context->functions.end())
    {
        context->lock.unlock();

        guest_function result = process_context->translate_function(process_context);

        context->lock.lock();

        context->functions[address] = result;

        context->lock.unlock();

        return result;
    }
    else
    {
        guest_function result = context->functions[address];

        context->lock.unlock();

        return result;
    }
}