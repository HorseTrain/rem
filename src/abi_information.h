#ifndef ABI_INFORMATION
#define ABI_INFORMATION

enum cpu_information
{
	x86_64,
	arm_64,
	unknown,
};

enum os_information
{
	_windows,
	_linux,
	_macos,
	_unknown
};

struct abi
{
	cpu_information cpu;
	os_information 	os;
};

os_information 	get_running_os();
cpu_information get_running_cpu();
abi 			get_abi();

#endif