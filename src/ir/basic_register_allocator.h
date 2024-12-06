#ifndef BASIC_REGISTER_ALLOCATOR_H
#define BASIC_REGISTER_ALLOCATOR_H

#include "ir.h"

struct basic_register_allocator_context;

enum register_mode
{
	none	= 0,
	read	= 1 << 0,
	write	= 1 << 1,

	read_write = read | write
};

enum lock_mode
{
	unlocked	= 0,
	basic_lock	= 1 << 0,
	force_lock  = 1 << 1,
};

struct host_register
{
	int				host_index;
	int				guest_offset;

	lock_mode		lock_data;

	register_mode	working_mode;

	int				hits;

	static void		set_lock_bit(host_register* guest, lock_mode mode);
	static void		unset_lock_bit(host_register* guest, lock_mode mode);
	static bool		is_locked(host_register* host);
	static bool		is_loaded(host_register* host);

	static bool		is_free(host_register* host);
};

struct guest_data
{
	uint64_t guest_type;
	uint64_t guest_count;
};

struct register_allocator_module
{
	basic_register_allocator_context*	allocator_unit;

	int									host_register_count;
	host_register*						host_registers;

	uint64_t							guest_type;
	uint64_t							guest_count;

	uint64_t							stack_offset;

	static void							emit_host_load(register_allocator_module* module, int host_register, int guest_offset, register_mode mode);
	static void							emit_host_unload(register_allocator_module* module, int host_register);
};

struct basic_register_allocator_context
{
	register_allocator_module*	gp_allocator;
	register_allocator_module*	vec_allocator;

	ir_operation_block*			result_ir;

	ir_operand					context_register;

	static void					run_pass(basic_register_allocator_context* result_register_allocator, ir_operation_block* result_ir, ir_operation_block* pre_allocated_code, int gp_count, guest_data gp_data, int vec_count, guest_data vec_data, ir_operand context_register);
};

#endif