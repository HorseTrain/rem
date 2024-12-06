#ifndef BIG_NUMBER_H
#define BIG_NUMBER_H

#include <inttypes.h>
#include <assert.h>
#include <string.h>

#include "numbers.h"

template <typename T>
int get_bit_count()
{
    return sizeof(T) * 8;
}

template <typename T, int C>
struct big_number
{
    T buffer[C];

    big_number()
    {
        for (int i = 0; i < C; ++i)
        {
            buffer[i] = 0;
        }
    }

    big_number(T source)
    {
        buffer[0] = source;

        for (int i = 1; i < C; ++i)
        {
            buffer[i] = 0;
        }
    }

    big_number<T,C> operator + (big_number<T,C> other)
    {
        big_number<T,C> result = 0;

        bool carry = false;

        for (int i = 0; i < C; ++i)
        {
            T x = buffer[i];
            T y = other.buffer[i] + carry;

            T r = x + y;

            carry = r < x;

            result.buffer[i] = r;
        }

        return result;
    }

    big_number<T,C> operator << (int other)
    {
        big_number<T,C> result = 0;

        T carry = 0;

        int bit_count = get_bit_count<T>();

        int r_offset = 0;

        while (other >= bit_count)
        {
            other -= bit_count;
            r_offset++;
        }

        for (int i = 0; i < C; ++i)
        {
            int to_place = r_offset + i;

            if (to_place >= C)
                continue;

            T x = buffer[i];

            T working = x << other;

            result.buffer[to_place] = working | carry;

            carry = x >> (bit_count - other);
        }

        return result;
    }

    big_number<T,C> operator * (big_number<T,C> other)
    {
        if (sizeof(T) == 1 && C == 1)
        {
            return buffer[0] * other.buffer[0];
        }

        big_number<uint8_t,C * sizeof(T)> working_result = 0;

        uint8_t* dx = (uint8_t*)buffer;
        uint8_t* dy = (uint8_t*)other.buffer;

        int size = C * sizeof(T);

        for (int x = 0; x < size; ++x)
        {
            for (int y = 0; y < size; ++y)
            {
                uint16_t raw_short_result = (uint16_t)dx[x] * (uint16_t)dy[y];

                big_number<uint8_t,C * sizeof(T)> to_be_shifted;

                to_be_shifted.buffer[0] = raw_short_result;
                to_be_shifted.buffer[1] = raw_short_result >> 8;

                int shift = ((x + y) * 8);

                to_be_shifted = to_be_shifted << shift;

                working_result = working_result + to_be_shifted;
            }
        }

        big_number<T,C> real_result;

        memcpy(real_result.buffer, working_result.buffer, sizeof(real_result));

        return real_result;
    }
};

static void test_multiplication()
{
    while (true)
    {
        uint64_t rx = create_random_number();
        uint64_t ry = create_random_number();

        uint64_t real_result = rx * ry;

        big_number<uint64_t,1> x = rx;
        big_number<uint64_t,1> y = ry;

        big_number<uint64_t,1> r = x * y;
        
        assert(r.buffer[0] == real_result);
    }
};

#endif