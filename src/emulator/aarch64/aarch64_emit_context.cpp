#include "aarch64_emit_context.h"
#include "emulator/ssa_emit_context.h"
#include "emulator/guest_process.h"

void aarch64_emit_context::create(guest_process* process, aarch64_emit_context* result, ssa_emit_context* ssa)
{
    result->ssa = ssa;
    result->raw_ir = ssa->ir;
    result->process = process;

    guest_register_store::create(&result->registers, ssa, process->guest_context_offset_data.context_size);
}

void aarch64_emit_context::init_context(aarch64_emit_context* ctx)
{
    ir_operation_block* ir = ctx->raw_ir;
    guest_process* process = ctx->process;

    ctx->context_pointer = ssa_emit_context::create_global(ctx->ssa, int64);

    ir_operation_block::emitds(ir, ir_get_argument, ctx->context_pointer,ir_operand::create_con(0));
    
    aarch64_context_offsets offsets = process->guest_context_offset_data;

    for (int i = 0; i < 32; ++i)
    {
        guest_register_store::create_register(&ctx->registers, offsets.x_offset + (i * 8), int64);
        guest_register_store::create_register(&ctx->registers, offsets.q_offset + (i * 16), int128);
    }

    guest_register_store::create_register(&ctx->registers, offsets.n_offset, int8);
    guest_register_store::create_register(&ctx->registers, offsets.z_offset, int8);
    guest_register_store::create_register(&ctx->registers, offsets.c_offset, int8);
    guest_register_store::create_register(&ctx->registers, offsets.v_offset, int8);

    guest_register_store::create_register(&ctx->registers, offsets.fpcr_offset, int64);
    guest_register_store::create_register(&ctx->registers, offsets.fpsr_offset, int64);

    guest_register_store::create_register(&ctx->registers, offsets.exclusive_address_offset, int64);
    guest_register_store::create_register(&ctx->registers, offsets.exclusive_value_offset, int64);
    
    guest_register_store::create_register(&ctx->registers, offsets.thread_local_0, int64);
    guest_register_store::create_register(&ctx->registers, offsets.thread_local_1, int64);

    emit_load_context(ctx);
}

void aarch64_emit_context::emit_load_context(aarch64_emit_context* ctx)
{
    ir_operation_block* ir = ctx->raw_ir;

    ctx->context_movement.push_back(ir_operation_block::emits(ir, ir_guest_load_context, ctx->context_pointer));
}

void aarch64_emit_context::emit_store_context(aarch64_emit_context* ctx)
{
    ir_operation_block* ir = ctx->raw_ir;

    ctx->context_movement.push_back(ir_operation_block::emits(ir, ir_guest_store_context, ctx->context_pointer));
}

bool aarch64_emit_context::basic_block_translated(aarch64_emit_context* ctx, uint64_t block)
{
    return ctx->basic_block_labels.find(block) != ctx->basic_block_labels.end();
}

void aarch64_emit_context::branch_long(aarch64_emit_context* ctx, ir_operand new_location)
{   
    ctx->branch_state = branch_type::long_branch;

    emit_store_context(ctx);

    ir_operation_block::emits(ctx->raw_ir, ir_close_and_return, new_location);
}

void aarch64_emit_context::emit_context_movement(aarch64_emit_context* ctx)
{
    ir_operation_block* ir = ctx->raw_ir;

    ir_operand context_pointer = ctx->context_pointer;

    for (auto i : ctx->context_movement)
    {
        for (int r = 0; r < ctx->registers.guest_context_size; ++r)
        {
            guest_register* working_register = &ctx->registers.guest_registers[r];

            if (working_register->free_guest)
                continue;

            if (working_register->mode == guest_usage::guest_usage_none)
                continue;

            if (i->data.instruction == ir_guest_load_context)
            {
                ir_operation_block::emitds(ir, ir_load, working_register->raw_register, context_pointer, ir_operand::create_con(r), i);
            }
            else
            {
                ir_operation_block::emits(ir, ir_store, context_pointer, ir_operand::create_con(r), working_register->raw_register, i);
            }
        }

        i->data.instruction = ir_no_operation;
    }
}

static int get_x_location(aarch64_emit_context* ctx, int index)
{
    int x_location = ctx->process->guest_context_offset_data.x_offset + (index * 8);

    return x_location;
}

static int get_v_location(aarch64_emit_context* ctx, int index)
{
    int v_location = ctx->process->guest_context_offset_data.q_offset + (index * 16);

    return v_location;
}

ir_operand aarch64_emit_context::get_x_raw(aarch64_emit_context* ctx, int index)
{
    assert(index >= 0 && index <= 31);

    ir_operand result = guest_register_store::request_guest_register(&ctx->registers,get_x_location(ctx, index));

    return result;
}

void aarch64_emit_context::set_x_raw(aarch64_emit_context* ctx, int index, ir_operand value)
{
    assert(index >= 0 && index <= 31);

    guest_register_store::write_to_guest_register(&ctx->registers, get_x_location(ctx, index), value);
}

ir_operand aarch64_emit_context::get_v_raw(aarch64_emit_context* ctx, int index)
{
    assert(index >= 0 && index <= 31);

    ir_operand result = guest_register_store::request_guest_register(&ctx->registers,get_v_location(ctx, index));

    return result;
}

void aarch64_emit_context::set_v_raw(aarch64_emit_context* ctx, int index, ir_operand value)
{
    assert(index >= 0 && index <= 31);

    guest_register_store::write_to_guest_register(&ctx->registers, get_v_location(ctx, index), value);
}

void aarch64_emit_context::set_context_reg_raw(aarch64_emit_context* ctx, int index, ir_operand value)
{
    guest_register_store::write_to_guest_register(&ctx->registers, index, value);
}

ir_operand aarch64_emit_context::get_context_reg_raw(aarch64_emit_context* ctx, int index)
{
    return guest_register_store::request_guest_register(&ctx->registers, index);
}
