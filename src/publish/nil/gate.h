#pragma once

#include <nil/xalt/MACROS.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct nil_gate_core;

struct nil_gate_core* nil_gate_core_create(void);
void nil_gate_core_destroy(struct nil_gate_core* core);
void nil_gate_core_commit(struct nil_gate_core const* core);

void nil_gate_core_set_runner_immediate(struct nil_gate_core* core);
void nil_gate_core_set_runner_soft_blocking(struct nil_gate_core* core);
void nil_gate_core_set_runner_non_blocking(struct nil_gate_core* core);
void nil_gate_core_set_runner_parallel_blocking(struct nil_gate_core* core, uint32_t thread_count);

struct nil_gate_port_info
{
    int (*eq)(const void*, const void*);
    void (*destroy)(void*);
};

struct nil_gate_mport
{
    void* const handle; // mutable_port
    const struct nil_gate_port_info info;
};

struct nil_gate_rport
{
    void* handle; // readonly_port
    struct nil_gate_port_info info;
};

// port creation
struct nil_gate_mport nil_gate_core_port(
    struct nil_gate_core* core,
    struct nil_gate_port_info info,
    void* initial_value
);
void nil_gate_port_set_value(struct nil_gate_mport port, void* new_value);
void nil_gate_port_unset_value(struct nil_gate_mport port);

struct nil_gate_input_ports
{
    uint8_t size;
    struct nil_gate_rport* ports;
};

struct nil_gate_output_port_infos
{
    uint8_t size;
    struct nil_gate_port_info* infos;
};

struct nil_gate_output_ports
{
    uint8_t size;
    struct nil_gate_rport* ports;
};

struct nil_gate_node_args
{
    const struct nil_gate_core* const core;

    struct {
        uint8_t size;
        const void* const* const  data;
    } inputs;

    struct {
        uint8_t size;
        void** const data; // to be filled by the node function
    } req_outputs;

    struct {
        uint8_t size;
        struct nil_gate_mport const* const ports;
     } opt_outputs;
};

struct nil_gate_node_info
{
    struct nil_gate_input_ports inputs;
    struct nil_gate_output_port_infos req_outputs;
    struct nil_gate_output_port_infos opt_outputs;
    void (*fn)(struct nil_gate_node_args* args);
};

void nil_gate_add_node(struct nil_gate_core const* core, struct nil_gate_node_info node_info, struct nil_gate_output_ports* output_ports);

struct nil_gate_rport nil_gate_mport_to_rport(struct nil_gate_mport port);
struct nil_gate_rport nil_gate_rport_to_rport(struct nil_gate_rport port);

#define nil_gate_to_rport(V) _Generic((V), struct nil_gate_mport: nil_gate_mport_to_rport, struct nil_gate_rport: nil_gate_rport_to_rport)(V)

// clang-format off
#define NIL_GATE_PORT_INFO(V)  (struct nil_gate_port_info){                     \
    .eq = NIL_XALT_CONCAT(nil_gate_port_eq_, V),                                \
    .destroy = NIL_XALT_CONCAT(nil_gate_port_destroy_, V)                       \
}

#define NIL_GATE_NODE_INPUT(...) (struct nil_gate_input_ports) {                \
    .ports = (struct nil_gate_rport[]){                                         \
        NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))      \
        (nil_gate_to_rport, __VA_ARGS__)                                        \
    },                                                                          \
    .size = NIL_XALT_NARG(NULL, __VA_ARGS__)                                    \
}

#define NIL_GATE_NODE_OUTPUT_PORT(...) (struct nil_gate_output_port_infos){     \
    .infos = (struct nil_gate_port_info[]){                                     \
        NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))      \
        (NIL_GATE_PORT_INFO, __VA_ARGS__)                                       \
    },                                                                          \
    .size = NIL_XALT_NARG(NULL, __VA_ARGS__)                                    \
}

#define NIL_GATE_NODE_OUTPUT(COUNT) (struct nil_gate_output_ports){             \
    .ports = (struct nil_gate_rport[]){                                         \
        NIL_XALT_CONCAT(NIL_XALT_REPEAT_, COUNT)(((struct nil_gate_rport){      \
            .handle = NULL, .info = {.eq = NULL, .destroy = NULL}               \
        }))                                                                     \
    },                                                                          \
    .size = (COUNT)                                                             \
}

#define NIL_GATE_NODE_INFO(FN, INPUTS, REQ_OUTPUTS, OPT_OUTPUTS) (struct nil_gate_node_info){   \
    .fn = (FN),                                                                                 \
    .inputs = (INPUTS),                                                                         \
    .req_outputs = (REQ_OUTPUTS),                                                               \
    .opt_outputs = (OPT_OUTPUTS)                                                                \
}
// clang-format on


#ifdef __cplusplus
}

#endif
