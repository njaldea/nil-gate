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
        printf("foo is here [index, value]: [%d, %d]\n", i, *(const int*)(props->inputs.data[i]));
    }

    if (props->req_outputs.size == 1)
    {
        int* output = malloc(sizeof(int));
        *output = *(const int*)(props->inputs.data[0]) * 2;
        props->req_outputs.data[0] = output;
    }
    printf("\n");


    if (props->opt_outputs.size == 1)
    {
        struct nil_gate_bports batch_ports = NIL_GATE_BATCH(1);
        nil_gate_core_batch(props->core, props->opt_outputs, &batch_ports);

        int* value = malloc(sizeof(int));
        *value = 1101;
        nil_gate_port_set_value(batch_ports.ports[0], value);
        nil_gate_core_batch_release(&batch_ports);
        nil_gate_core_commit(props->core);
    }
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

    struct nil_gate_output_ports node_outputs_1 = NIL_GATE_NODE_OUTPUT(1);
    nil_gate_add_node(
        core,
        NIL_GATE_NODE_INFO(
            &bar,
            NIL_GATE_NODE_INPUT(port1, port2),
            NIL_GATE_NODE_NO_OUTPUT_PORT(),
            NIL_GATE_NODE_OUTPUT_PORT(int)
        ),
        &node_outputs_1
    );

    nil_gate_add_node(
        core,
        NIL_GATE_NODE_INFO(
            &bar,
            NIL_GATE_NODE_INPUT(node_outputs_1.ports[0], node_outputs_1.ports[0]),
            NIL_GATE_NODE_NO_OUTPUT_PORT(),
            NIL_GATE_NODE_NO_OUTPUT_PORT()
        ),
        NULL
    );
    nil_gate_core_commit(core);
    {
        int* new_value = malloc(sizeof(int));
        *new_value = 20;
        nil_gate_port_set_value(port1, new_value);
        nil_gate_core_commit(core);
    }
    {
        struct nil_gate_bports batch_ports = NIL_GATE_BATCH(2);
        nil_gate_core_batch(core, (struct nil_gate_mports){
            .size = 2,
            .ports = (struct nil_gate_mport[]){port1, port2}
        }, &batch_ports);

        int* new_value = malloc(sizeof(int));
        *new_value = 130;
        nil_gate_port_set_value(batch_ports.ports[0], new_value);

        nil_gate_core_batch_release(&batch_ports);
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
