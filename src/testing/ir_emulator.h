#ifndef IR_EMULATOR_H
#define IR_EMULATOR_H

#include <unordered_map>

#include "ir/ir.h"

struct ir_emulator
{
    std::unordered_map<uint64_t, uint64_t>          working_registers;
    uint64_t*                                       arguments;
    intrusive_linked_list_element<ir_operation>*    working_instruction_element;                                

    static void                                     create(ir_emulator* ir_emulator_context);
    static void                                     destroy(ir_emulator* to_destroy);
    static uint64_t                                 execute(ir_emulator* ir_emulator_context, ir_operation_block* to_execute, uint64_t* arguments);
};

#endif