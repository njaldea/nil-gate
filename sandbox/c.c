#include <nil/gate.h>

#include <stdio.h>
#include <stdlib.h>

void nil_gate_port_destroy_int(void* value)
{
    free(value);
}

int nil_gate_port_eq_int(const void* l, const void* r)
{
    return *(const int*)(l) == *(const int*)(r) ? 1 : 0;
}

void bar(struct nil_gate_node_args* props)
{
    for (int i = 0; i < props->inputs.size; ++i)
    {
        printf("foo is here: %d %d\n", i, *(const int*)(props->inputs.data[i]));
    }
    int* output = malloc(sizeof(int));
    *output = *(const int*)(props->inputs.data[0]) * 2;
    props->req_outputs.data[0] = output;
    printf("\n");
}


int main(void)
{
    struct nil_gate_core* core = nil_gate_core_create();
    nil_gate_core_set_runner_soft_blocking(core);

    int* value = malloc(sizeof(int));
    *value = 10;
    struct nil_gate_mport port1 = nil_gate_core_port(core, NIL_GATE_PORT_INFO(int), value);
    
    value = malloc(sizeof(int));
    *value = 11;
    struct nil_gate_mport port2 = nil_gate_core_port(core, NIL_GATE_PORT_INFO(int), value);

    struct nil_gate_output_ports node_outputs_1 = NIL_GATE_NODE_OUTPUT(2);
    nil_gate_add_node(
        core,
        NIL_GATE_NODE_INFO(
            &bar,
            NIL_GATE_NODE_INPUT(port1, port2),
            NIL_GATE_NODE_OUTPUT_PORT(int),
            NIL_GATE_NODE_OUTPUT_PORT(int)
        ),
        &node_outputs_1
    );

    struct nil_gate_output_ports node_outputs_2 = NIL_GATE_NODE_OUTPUT(4);
    nil_gate_add_node(
        core,
        NIL_GATE_NODE_INFO(
            &bar,
            NIL_GATE_NODE_INPUT(node_outputs_1.ports[0], node_outputs_1.ports[0]),
            NIL_GATE_NODE_OUTPUT_PORT(int, int),
            NIL_GATE_NODE_OUTPUT_PORT(int, int)
        ),
        &node_outputs_2
    );
    nil_gate_core_commit(core);
    {
        int* new_value = malloc(sizeof(int));
        *new_value = 20;
        nil_gate_port_set_value(port1, new_value);
        nil_gate_core_commit(core);
    }
    {
        nil_gate_port_unset_value(port1);
        nil_gate_core_commit(core);
    }
    {
        int* new_value = malloc(sizeof(int));
        *new_value = 30;
        nil_gate_port_set_value(port1, new_value);
        nil_gate_core_commit(core);
    }
    nil_gate_core_destroy(core);
    return 0;
}
