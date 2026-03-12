#include <nil/gate.h>

#include <stdio.h>
#include <stdlib.h>

void nil_gate_port_destroy_int(void* value)
{
    printf("destroying %p\n", value);
    free(value);
}

int nil_gate_port_eq_int(const void* l, const void* r)
{
    return *(const int*)(l) == *(const int*)(r) ? 1 : 0;
}

void bar(struct nil_gate_node_args const* props)
{
    for (int i = 0; i < props->inputs.size; ++i)
    {
        printf("foo is here [index, value]: [%d, %d]\n", i, *(const int*)(props->inputs.data[i]));
    }

    if (props->outputs.size == 1)
    {
        int* output = malloc(sizeof(int));
        *output = *(const int*)(props->inputs.data[0]) * 2;
        nil_gate_mport_set_value(props->outputs.ports[0], output);
    }
    printf("\n");


    if (props->outputs.size == 2)
    {
        // struct nil_gate_bports batch_ports = NIL_GATE_BATCH(1);
        // nil_gate_core_batch(props->core, props->opt_outputs, &batch_ports);

        // int* value = malloc(sizeof(int));
        // *value = 1101;
        // nil_gate_port_set_value(batch_ports.ports[0], value);
        // nil_gate_core_batch_release(&batch_ports);
        // nil_gate_core_commit(props->core);
    }
}

struct nil_gate_eport port1; // NOLINT
struct nil_gate_eport port2; // NOLINT

void gate_apply_1(struct nil_gate_graph* graph, void* context)
{
    printf("gate_apply_1\n");
    (void)graph;
    (void)context;

    int* value = malloc(sizeof(int));
    *value = 10;
    port1 = nil_gate_graph_port(graph, NIL_GATE_PORT_INFO(int), value);

    value = malloc(sizeof(int));
    *value = 11;
    port2 = nil_gate_graph_port(graph, NIL_GATE_PORT_INFO(int), value);

    struct nil_gate_rports node_outputs_1 = NIL_GATE_NODE_OUTPUTS(1);
    nil_gate_graph_node(
        graph,
        (struct nil_gate_node_info){
            .exec=bar,
            .inputs=NIL_GATE_RPORTS(port1, port2),
            .outputs=NIL_GATE_PORT_INFOS(int),
            .output_handles=&node_outputs_1,
            .context=NULL,
            .destroy_context=NULL
        }
    );

    nil_gate_graph_node(
        graph,
        (struct nil_gate_node_info){
            .exec=bar,
            .inputs=NIL_GATE_RPORTS(node_outputs_1.ports[0], node_outputs_1.ports[0]),
            .outputs=NIL_GATE_NO_PORT_INFOS(),
            .output_handles=NULL,
            .context=NULL,
            .destroy_context=NULL
        }
    );
}

struct port_set_context{
    int* data;
    struct nil_gate_eport port;
};

void setter(struct nil_gate_graph* graph, struct port_set_context* context)
{
    (void)graph;
    nil_gate_port_set_value(context->port, context->data);
}

void gate_apply_2(struct nil_gate_graph* graph, void* context)
{
    printf("gate_apply_2\n");
    (void)graph;
    (void)context;

    int* new_value = malloc(sizeof(int));
    *new_value = 20;
    // need to post
    nil_gate_port_set_value(port1, new_value);
}

void gate_apply_3(struct nil_gate_graph* graph, void* context)
{
    printf("gate_apply_3\n");
    (void)graph;
    (void)context;

    // need to post
    nil_gate_port_unset_value(port1);
}

void gate_apply_4(struct nil_gate_graph* graph, void* context)
{
    printf("gate_apply_4\n");
    (void)graph;
    (void)context;

    int* new_value = malloc(sizeof(int));
    *new_value = 30;
    // need to post
    nil_gate_port_set_value(port1, new_value);
}

int main(void)
{
    struct nil_gate_core* core = nil_gate_core_create();
    nil_gate_core_set_runner_soft_blocking(core);
    nil_gate_core_apply(core, (struct nil_gate_core_callable){.exec=gate_apply_1, .context=NULL, .destroy_context=NULL});
    nil_gate_core_apply(core, (struct nil_gate_core_callable){.exec=gate_apply_2, .context=NULL, .destroy_context=NULL});
    nil_gate_core_apply(core, (struct nil_gate_core_callable){.exec=gate_apply_3, .context=NULL, .destroy_context=NULL});
    nil_gate_core_apply(core, (struct nil_gate_core_callable){.exec=gate_apply_4, .context=NULL, .destroy_context=NULL});
    nil_gate_core_destroy(core);
    return 0;
}
