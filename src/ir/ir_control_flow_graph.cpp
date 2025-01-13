#include "ir.h"

static ir_control_flow_node* create_and_insert_node(intrusive_linked_list<ir_control_flow_node*>* list, intrusive_linked_list_element<ir_operation>* first_instruction, intrusive_linked_list_element<ir_operation>* block_final_instruction)
{
    ir_control_flow_node* result = arena_allocator::allocate_struct<ir_control_flow_node>(list->allocator);

    result->entry_instruction = first_instruction;
    result->final_instruction = block_final_instruction;

    intrusive_linked_list<ir_control_flow_node*>::insert_element(list, result);

    return result;
}

static bool is_label(ir_operation* operation)
{
    switch (operation->instruction)
    {
        case ir_mark_label:
            return true;
    }

    return false;
}

static bool is_jump(ir_operation* operation)
{
    switch (operation->instruction)
    {
        case ir_jump_if:
        case ir_close_and_return:
        case ir_table_jump:
            return true;
    }

    return false;
}

static void get_linier_nodes(ir_control_flow_graph* result)
{
    ir_operation_block* source_ir = result->source_ir;

    ir_control_flow_node* working_node = create_and_insert_node(result->linier_nodes, source_ir->operations->first, source_ir->operations->last);

    for (auto i = source_ir->operations->first; i != source_ir->operations->last; i = i->next)
    {
        ir_operation working_operation = i->data;

        if (is_jump(&working_operation) || (i->next != nullptr && is_label(&i->next->data)))
        {
            working_node->final_instruction = i;

            working_node = create_and_insert_node(result->linier_nodes, i->next, source_ir->operations->last);
        }
    }
}

ir_control_flow_graph* ir_control_flow_graph::create(ir_operation_block* source)
{
    ir_control_flow_graph* result = arena_allocator::allocate_struct<ir_control_flow_graph>(source->allocator);

    result->source_ir = source;
    result->linier_nodes = intrusive_linked_list<ir_control_flow_node*>::create(source->allocator,nullptr, nullptr);

    get_linier_nodes(result);

    return result;
}
