#ifndef DEBUGGING_H
#define DEBUGGING_H
#include <iostream>
#include <exception>

#define throw_error() do { std::cout << std::dec << "ERROR IN FILE " << __FILE__ << ":" << __LINE__ << std::endl; while (1); } while (0)

#endif