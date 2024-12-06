#include "abi_information.h"

os_information get_running_os()
{
    #if defined __linux__
        return os_information::_linux;
    #elif defined _WIN32
        return os_information::_windows;
    #elif define __APPLE__
        return os_information::_macos;
    #endif

    return os_information::_unknown;
}

cpu_information get_running_cpu()
{
    #if (defined __x86_64__) || (defined _M_X64)
        return cpu_information::x86_64;
    #elif (defined __aarch64__) || (defined _M_ARM64)
        return cpu_information::arm_64;
    #endif

    return cpu_information::unknown; 
}

abi get_abi()
{
    abi result;

    result.cpu = get_running_cpu();
    result.os = get_running_os();

    return result;
}