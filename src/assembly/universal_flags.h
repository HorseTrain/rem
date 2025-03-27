#ifndef UNIVERSAL_FLAGS_H
#define UNIVERSAL_FLAGS_H

enum compiler_flags
{
    check_undefined_behavior    = 1ULL  << 0,
    optimize_basic_ssa          = 1ULL  << 1,
    optimize_group_pool_ssa     = 1ULL  << 2
};

#endif