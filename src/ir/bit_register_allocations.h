#ifndef BIT_REGISTER_ALLOCATIONS
#define BIT_REGISTER_ALLOCATIONS

#include <inttypes.h>

enum bit_register_allocations : uint64_t
{
	gp_allocation		= 1ULL << 48,
	vec_allocation		= 1ULL << 49,
	scrap_allocation	= 1ULL << 50
};

#endif