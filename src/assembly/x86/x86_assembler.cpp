#include <string>

#include "x86_assembler.h"
#include "ir/checks.h"
#include "xbyak/xbyak.h"
#include "tools/bit_tools.h"

#define ONE_MB 1ULL * 1024 * 1024

#define RAX 0
#define RCX 1
#define RDX 2
#define RSP 4

//Strings introduce memory allocations,
//and memory allocations are EVIL.

//TODO: using strings is the most elegant
//solution, but i do not want to use them
//forever.
static std::string create_label(ir_operand value)
{
	assert(ir_operand::is_constant(&value));

	return "l_" + std::to_string(value.value);
}

template <typename T>
static T create_operand(ir_operand value)
{
	assert(ir_operand::is_register(&value));

	int reg = value.value;

	return T(reg);
}

template <typename T>
static T convert_operand(Xbyak::Operand value)
{
	return T(value.getIdx());
}

static Xbyak::Operand create_operand(ir_operand value)
{
	assert(ir_operand::is_register(&value));

	switch (ir_operand::get_raw_size(&value))
	{
	case int8: return Xbyak::Reg8(value.value);
	case int16: return Xbyak::Reg16(value.value);
	case int32: return Xbyak::Reg32(value.value);
	case int64: return Xbyak::Reg64(value.value);
	case int128: return Xbyak::Xmm(value.value);
	}

	assert(false);

	throw 0;
}

static void assert_is_x86_register(ir_operand working_operand, int register_index)
{
	assert(working_operand.value == register_index);
}

static void assert_valid_mul_div_operation(ir_operation* operation)
{
	assert_operand_count(operation, 2, 2);
	
	assert_is_x86_register(operation->destinations[0], RDX);
	assert_is_x86_register(operation->destinations[1], RAX);
	
	assert_is_x86_register(operation->sources[0], RAX);
	assert_is_register(operation->sources[1]);
}

static void assert_valid_c_operation(ir_operation* operation)
{
	assert_operand_count(operation, 1, 1);

	assert_is_x86_register(operation->destinations[0], RDX);
	assert_is_x86_register(operation->sources[0], RAX);
}

static void assert_valid_unary_operation(ir_operation* operation)
{
	assert_operand_count(operation, 1, 1);

	assert_all_registers(operation);
	assert_same_registers(operation->destinations[0], operation->sources[0]);
}

void assemble_x86_64_code(void** result_code, uint64_t* result_code_size, ir_operation_block* source_ir)
{
	arena_allocator* allocator = source_ir->allocator;

	*result_code = arena_allocator::allocate_recursive(allocator, ONE_MB);
	Xbyak::CodeGenerator c(ONE_MB, *result_code);

	for (auto i = source_ir->operations->first; i != source_ir->operations->last; i = i->next)
	{
		ir_operation	working_operation = i->data;
		ir_instructions instruction = (ir_instructions)working_operation.instruction;

		switch (instruction)
		{
		case ir_negate:						c.neg(create_operand(working_operation.destinations[0])); 	assert_valid_unary_operation(&working_operation); 	break;
		case ir_incrament:					c.inc(create_operand(working_operation.destinations[0])); 	assert_valid_unary_operation(&working_operation); 	break;
		case ir_bitwise_not:				c.not_(create_operand(working_operation.destinations[0])); 	assert_valid_unary_operation(&working_operation); 	break;

		case x86_cwd:						c.cwd(); 													assert_valid_c_operation(&working_operation); 		break;
		case x86_cqo:						c.cqo(); 													assert_valid_c_operation(&working_operation); 		break;
		case x86_cdq:						c.cdq(); 													assert_valid_c_operation(&working_operation); 		break;

		case ir_divide_signed:				c.idiv(create_operand(working_operation.sources[1])); 		assert_valid_mul_div_operation(&working_operation);	break;
		case ir_divide_unsigned:			c.div(create_operand(working_operation.sources[1])); 		assert_valid_mul_div_operation(&working_operation);	break;
		case ir_multiply_hi_signed:			c.imul(create_operand(working_operation.sources[1])); 		assert_valid_mul_div_operation(&working_operation);	break;
		case ir_multiply_hi_unsigned:		c.mul(create_operand(working_operation.sources[1])); 		assert_valid_mul_div_operation(&working_operation);	break;

		case ir_mark_label:
		{
			if (working_operation.sources.count == 0)
			{
				break;
			}

			assert_operand_count(&working_operation, 0, 1);
			assert_is_constant(working_operation.sources[0]);

			std::string label = create_label(working_operation.sources[0]);

			c.L(label);

			c.nop();

		}; break;

		case ir_jump_if:
		{
			assert_operand_count(&working_operation, 0, 2);

			ir_operand label_operand = working_operation.sources[0];
			ir_operand condition = working_operation.sources[1];

			assert_is_constant(label_operand);

			std::string xbyak_label = create_label(label_operand);

			if (ir_operand::is_constant(&condition))
			{
				if (condition.value)
				{
					c.jmp(xbyak_label);
				}
			}
			else
			{
				Xbyak::Operand xbyak_condition = create_operand(condition);

				c.cmp(xbyak_condition, 0);
				c.jne(xbyak_label);
			}

		}; break;

		case ir_shift_left:
		case ir_shift_right_signed:
		case ir_shift_right_unsigned:
		case ir_rotate_right:
		{
			ir_operand* destinations = working_operation.destinations.data;
			ir_operand* sources = working_operation.sources.data;

			assert_operand_count(&working_operation, 1, 2);
			
			assert_is_register(destinations[0]);
			assert_is_register(sources[0]);

			assert(ir_operand::are_equal(destinations[0], sources[0]));

			Xbyak::Operand dn = create_operand(destinations[0]);

			if (ir_operand::is_register(&sources[1]))
			{
				Xbyak::Reg8 m = create_operand<Xbyak::Reg8>(sources[1]);
				assert_is_x86_register(sources[1], RCX);

				switch (instruction)
				{
					case ir_shift_left:				c.shl(dn, m); break;
					case ir_shift_right_signed:		c.sar(dn, m); break;
					case ir_shift_right_unsigned:	c.shr(dn, m); break;
					case ir_rotate_right:			c.ror(dn, m); break;
					default: throw 0;
				}
			}
			else
			{
				int shift = sources[1].value;

				switch (instruction)
				{
					case ir_shift_left:				c.shl(dn, shift); break;
					case ir_shift_right_signed:		c.sar(dn, shift); break;
					case ir_shift_right_unsigned:	c.shr(dn, shift); break;
					case ir_rotate_right:			c.ror(dn, shift); break;
					default: throw 0;
				}
			}
		}; break;

		case ir_double_shift_right:
		{
			ir_operand* destinations = working_operation.destinations.data;
			ir_operand* sources = working_operation.sources.data;

			assert_operand_count(&working_operation, 1, 3);
			assert_same_registers(destinations[0], sources[0]);
			assert_is_register(sources[1]);
			assert_is_x86_register(sources[2], RCX);

			assert(destinations[0].value == sources[0].value);
			assert(destinations[0].meta_data == sources[0].meta_data);

			Xbyak::Operand dn = create_operand(destinations[0]);
			Xbyak::Operand m = create_operand(sources[1]);

			Xbyak::Reg8 cl = create_operand<Xbyak::Reg8>(sources[2]);

			c.shrd(dn, m.getReg(), cl);
		}; break;

		case ir_assert_true:
		case ir_assert_false:
		{
			ir_operand* destinations = working_operation.destinations.data;
			ir_operand* sources = working_operation.sources.data;

			assert_operand_count(&working_operation, 0, 1);

			Xbyak::Operand check = create_operand(sources[0]);

			c.cmp(check, 0);

			Xbyak::Label end;

			if (instruction == ir_assert_true)
			{
				c.jne(end);
			}
			else
			{
				c.je(end);
			}

			c.mov(c.rax,0);
			c.mov(c.rax,c.ptr[c.rax]);

			c.L(end);
		}; break;

		case ir_conditional_select:
		{
			ir_operand* destinations = working_operation.destinations.data;
			ir_operand* sources = working_operation.sources.data;
			
			assert_operand_count(&working_operation, 1, 3);
			assert_is_register(destinations[0]);

			Xbyak::Operand condition = create_operand(sources[0]);

			c.cmp(condition, 0);

			switch (destinations[0].meta_data)
			{
				case int16: c.cmovne(Xbyak::Reg16(destinations[0].value), Xbyak::Reg16(sources[1].value)); c.cmove(Xbyak::Reg16(destinations[0].value), Xbyak::Reg16(sources[2].value)); break;
				case int32: c.cmovne(Xbyak::Reg32(destinations[0].value), Xbyak::Reg32(sources[1].value)); c.cmove(Xbyak::Reg32(destinations[0].value), Xbyak::Reg32(sources[2].value)); break;
				case int64: c.cmovne(Xbyak::Reg64(destinations[0].value), Xbyak::Reg64(sources[1].value)); c.cmove(Xbyak::Reg64(destinations[0].value), Xbyak::Reg64(sources[2].value)); break;
				default: throw 0;
			}
		}; break;

		case ir_compare_equal:
		case ir_compare_greater_equal_signed:
		case ir_compare_greater_equal_unsigned:
		case ir_compare_greater_signed:
		case ir_compare_greater_unsigned:
		case ir_compare_less_equal_signed:
		case ir_compare_less_equal_unsigned:
		case ir_compare_less_signed:
		case ir_compare_less_unsigned:
		case ir_compare_not_equal:
		{
			ir_operand* destinations = working_operation.destinations.data;
			ir_operand* sources = working_operation.sources.data;

			assert_is_register(destinations[0]);
			assert_is_register(sources[0]);

			//assert_all_registers(&working_operation);

			assert(destinations[0].meta_data == sources[0].meta_data);
			assert(destinations[0].meta_data == sources[1].meta_data);

			if (ir_operand::is_constant(&sources[1]))
			{
				uint64_t constant = ir_operand::get_masked_constant(&sources[1]);

				uint64_t working_size = ir_operand::get_raw_size(&working_operation.sources[1]);

				if (working_size <= int32)
				{
					assert(constant <= create_int_max(working_size));	
				}
				else
				{
					assert(constant <= create_int_max(int32));	
				}

				c.cmp(create_operand(sources[0]), constant);
			}
			else
			{
				c.cmp(create_operand(sources[0]), create_operand(sources[1]));
			}

			switch (instruction)
			{
			case ir_compare_equal:					c.sete(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_greater_equal_signed:	c.setge(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_greater_equal_unsigned:	c.setae(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_greater_signed:			c.setg(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_greater_unsigned:		c.seta(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_less_equal_signed:		c.setle(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_less_equal_unsigned:	c.setbe(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_less_signed:			c.setl(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_less_unsigned:			c.setb(create_operand<Xbyak::Reg8>(destinations[0])); break;
			case ir_compare_not_equal:				c.setne(create_operand<Xbyak::Reg8>(destinations[0])); break;
			}

			ir_operand d = destinations[0];
			d.meta_data = int64;
			
			c.and_(create_operand(d), 1);

		}; break;

		case ir_add:
		case ir_subtract:
		case ir_bitwise_and:
		case ir_bitwise_or:
		case ir_bitwise_exclusive_or:
		case ir_multiply:
		{
			ir_operand* destinations = working_operation.destinations.data;
			ir_operand* sources = working_operation.sources.data;

			assert_is_register(destinations[0]);
			assert_same_registers(destinations[0], sources[0]);

			assert(ir_operand::is_register(&destinations[0]));
			assert(ir_operand::is_register(&sources[0]));

			Xbyak::Operand dn = create_operand(destinations[0]);

			if (ir_operand::is_constant(&sources[1]))
			{
				uint64_t imm = sources[1].value;

				assert(imm <= INT32_MAX);

				switch (instruction)
				{
					case ir_add: c.add(dn, imm); break;
					case ir_subtract: c.sub(dn, imm); break;
					case ir_bitwise_and: c.and_(dn, imm); break;
					case ir_bitwise_or: c.or_(dn, imm); break;
					case ir_bitwise_exclusive_or: c.xor_(dn, imm); break;
					default: assert(false); throw 0;
				}
			}
			else
			{
				assert(destinations[0].value == sources[0].value);
				assert(destinations[0].meta_data == sources[0].meta_data);
				assert(destinations[0].meta_data == sources[1].meta_data);

				Xbyak::Operand m = create_operand(sources[1]);

				switch (instruction)
				{
					case ir_add: c.add(dn, m); break;
					case ir_subtract: c.sub(dn, m); break;
					case ir_bitwise_and: c.and_(dn, m); break;
					case ir_bitwise_or: c.or_(dn, m); break;
					case ir_bitwise_exclusive_or: c.xor_(dn, m); break;
					case ir_multiply: c.imul(Xbyak::Reg(dn.getIdx(), dn.getKind()), m); break;
					default: assert(false); throw 0;
				}
			}
		}; break;

		case ir_no_operation: /*NOP*/ break;

		case ir_open_context:
		{
			ir_operand size = working_operation.sources[0];

			assert(ir_operand::is_constant(&size));

			uint64_t offset = size.value;

			c.sub(c.rsp, offset);

		}; break;

		case ir_close_and_return:
		{
			ir_operand value = working_operation.sources[0];
			ir_operand context_size = working_operation.sources[1];

			uint64_t offset = context_size.value;

			assert(ir_operand::is_register(&value));
			assert(ir_operand::is_constant(&context_size));

			c.add(c.rsp, offset);

			if (value.value != c.rax.getIdx())
			{
				c.mov(c.rax, create_operand<Xbyak::Reg64>(value));
			}

			c.ret();

		}; break;

		case ir_load:
		{
			ir_operand destination = working_operation.destinations[0];
			ir_operand base_address = working_operation.sources[0];

			Xbyak::Operand result_operand = create_operand(destination);
			Xbyak::Address address_operand = Xbyak::Address(0);

			if (working_operation.sources.count == 1)
			{
				address_operand = c.ptr[create_operand<Xbyak::Reg64>(base_address)];
			}
			else
			{
				ir_operand address_offset = working_operation.sources[1];

				if (ir_operand::is_constant(&address_offset))
				{
					address_operand = c.ptr[create_operand<Xbyak::Reg64>(base_address) + address_offset.value];
				}
				else
				{
					address_operand = c.ptr[create_operand<Xbyak::Reg64>(base_address) + create_operand<Xbyak::Reg64>(address_offset)];
				}
			}

			if (ir_operand::is_vector(&destination))
			{
				c.movups(convert_operand<Xbyak::Xmm>(result_operand), address_operand);
			}
			else
			{
				c.mov(result_operand, address_operand);
			}
		}; break;

		case ir_store:
		{
			ir_operand base_address = working_operation.sources[0];
			ir_operand to_store;

			Xbyak::Address address_operand = Xbyak::Address(0);

			if (working_operation.sources.count == 3)
			{
				ir_operand offset_address = working_operation.sources[1];

				if (ir_operand::is_constant(&offset_address))
				{
					address_operand = c.ptr[create_operand<Xbyak::Reg64>(working_operation.sources[0]) + offset_address.value];
				}
				else
				{
					address_operand = c.ptr[create_operand<Xbyak::Reg64>(working_operation.sources[0]) + create_operand<Xbyak::Reg64>(working_operation.sources[1])];
				}

				to_store = working_operation.sources[2];
			}
			else if (working_operation.sources.count == 2)
			{
				address_operand = c.ptr[create_operand<Xbyak::Reg64>(working_operation.sources[0])];

				to_store = working_operation.sources[1];
			}
			else
			{
				assert(false);

				throw 0;
			}

			if (ir_operand::is_vector(&to_store))
			{
				c.movups(address_operand, create_operand<Xbyak::Xmm>(to_store));
			}
			else
			{
				c.mov(address_operand, create_operand(to_store));
			}
		}; break;

		case ir_move:
		{
			ir_operand destination = working_operation.destinations[0];
			ir_operand source = working_operation.sources[0];

			assert(ir_operand::get_raw_size(&destination) == ir_operand::get_raw_size(&source));

			if (ir_operand::is_vector(&destination))
			{
				c.movaps(create_operand<Xbyak::Xmm>(destination), create_operand<Xbyak::Xmm>(source));
			}
			else
			{
				if (ir_operand::is_constant(&source))
				{
					uint64_t working = ir_operand::get_masked_constant(&source);

					c.mov(create_operand<Xbyak::Reg64>(destination), working);
				}
				else
				{
					c.mov(create_operand(destination), create_operand(source));
				}
			}
		}; break;

		case ir_sign_extend:
		{
			ir_operand destination = working_operation.destinations[0];
			ir_operand source = working_operation.sources[0];

			if (ir_operand::get_raw_size(&source) == ir_operand_meta::int32)
			{
				c.movsxd(create_operand<Xbyak::Reg64>(destination), create_operand(source));
			}
			else
			{
				c.movsx(create_operand(destination).getReg(), create_operand(source));
			}
		}; break;

		default:
		{
			std::cout << "UNDEFINED X86 INSTRUCTION " << std::endl;

			throw 0;
		}; break;
		}
	}

	*result_code_size = c.getSize();
}

void assemble_x86_abi_caller_code(void* result_code, uint64_t* result_code_size, abi abi_information)
{
	assert(abi_information.cpu == cpu_information::x86_64);

	uint64_t size = 1 * 1024;

	Xbyak::CodeGenerator c(size, result_code);

	for (int i = 0; i < 16; ++i)
	{
		c.push(Xbyak::Reg64(i));

		c.sub(c.rsp, 16);
		c.movups(c.ptr[c.rsp], Xbyak::Xmm(i));
	}

	switch (abi_information.os)
	{
		case _macos:
		case _linux:
		{
			c.push(c.rsi);
			c.call(c.rdi);
		}; break;

		case _windows:
		{
			c.push(c.rdx);
			c.call(c.rcx);
		}; break;
	}

	c.add(c.rsp, 8);

	for (int i = 15; i != -1; --i)
	{
		c.movups(c.ptr[c.rsp], Xbyak::Xmm(i));
		c.add(c.rsp, 16);

		if (i == 0 || i == 4)
		{
			c.add(c.rsp, 8);
		}
		else
		{
			c.pop(Xbyak::Reg64(i));
		}
	}

	c.ret();

	*result_code_size = c.getSize();
}