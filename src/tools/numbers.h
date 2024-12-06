#ifndef NUMBERS_H
#define NUMBERS_H

#include <inttypes.h>
#include <random>

static uint64_t create_random_number()
{
    uint64_t result ;

    for (int i = 0; i < 8; ++i)
    {
        result |= ((uint64_t)rand() & 255) << (i * 8);
    }

    return result;
}

#endif