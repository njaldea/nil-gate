#pragma once

#include <nil/xalt/MACROS.h>

// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stddef.h>

// NOLINTNEXTLINE(hicpp-deprecated-headers,modernize-deprecated-headers)
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // core types

    struct nil_gate_core;
    struct nil_gate_graph;

    struct nil_gate_port_info
    {
        int (*eq)(const void*, const void*);
        void (*destroy)(void*);
    };

    struct nil_gate_bport
    {
        void* handle; // batch_port
        struct nil_gate_port_info info;
    };

    struct nil_gate_rport
    {
        void* handle; // readonly_port
        struct nil_gate_port_info info;
        int port_type; // 0 core port, 1 node port
    };

    struct nil_gate_mport
    {
        void* handle; // mutable_port
        struct nil_gate_port_info info;
        int port_type; // 0 core port, 1 node port
    };

    // core operations

    struct nil_gate_core* nil_gate_core_create(void);
    void nil_gate_core_destroy(struct nil_gate_core* core);
    void nil_gate_core_commit(struct nil_gate_core const* core);

    void nil_gate_core_set_runner_immediate(struct nil_gate_core* core);
    void nil_gate_core_set_runner_soft_blocking(struct nil_gate_core* core);
    void nil_gate_core_set_runner_non_blocking(struct nil_gate_core* core);
    void nil_gate_core_set_runner_parallel_blocking(
        struct nil_gate_core* core,
        uint32_t thread_count
    );
    void nil_gate_core_unset_runner(struct nil_gate_core* core);

    void nil_gate_core_apply(struct nil_gate_core* core, void (*callable)(struct nil_gate_graph*));

    struct nil_gate_mport nil_gate_graph_port(
        struct nil_gate_graph* graph,
        struct nil_gate_port_info info,
        void* initial_value
    );
    // port operations

    void nil_gate_mport_set_value(struct nil_gate_mport port, void* new_value);
    void nil_gate_mport_unset_value(struct nil_gate_mport port);

    struct nil_gate_port_infos
    {
        uint8_t size;
        struct nil_gate_port_info const* infos;
    };

    struct nil_gate_rports
    {
        uint8_t size;
        struct nil_gate_rport* ports;
    };

    // add distinction between mport and port
    // mport is mutable port (direct mutation)
    // port is a core port (queue mutation)
    struct nil_gate_mports
    {
        uint8_t size;
        struct nil_gate_mport const* ports;
    };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define nil_gate_port_set_value(PORT, VALUE)                                                       \
    _Generic((PORT), struct nil_gate_mport: nil_gate_mport_set_value)(PORT, VALUE)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define nil_gate_port_unset_value(PORT)                                                            \
    _Generic((PORT), struct nil_gate_mport: nil_gate_mport_unset_value)(PORT)

    // node setup

    struct nil_gate_node_args;

    // node execution

    struct nil_gate_node_args
    {
        struct nil_gate_graph const* graph;

        struct inputs
        {
            uint8_t size;
            const void* const* data;
        } inputs;

        struct req_outputs
        {
            uint8_t size;
            void** data; // to be filled by the node function
        } req_outputs;

        struct nil_gate_mports opt_outputs;
    };

    void nil_gate_graph_node(
        struct nil_gate_graph const* graph,
        void (*fn)(struct nil_gate_node_args const* args),
        struct nil_gate_rports inputs,
        struct nil_gate_port_infos req_outputs,
        struct nil_gate_port_infos opt_outputs,
        struct nil_gate_rports* outputs
    );

    struct nil_gate_rport nil_gate_mport_to_rport(struct nil_gate_mport port);
    struct nil_gate_rport nil_gate_rport_to_rport(struct nil_gate_rport port);
    struct nil_gate_mport nil_gate_mport_to_mport(struct nil_gate_mport port);

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define nil_gate_to_rport(V)                                                                                       \
    _Generic((V), struct nil_gate_mport: nil_gate_mport_to_rport, struct nil_gate_rport: nil_gate_rport_to_rport)( \
        V                                                                                                          \
    )

    // clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_PORT_INFO(V)  (struct nil_gate_port_info){                     \
    .eq = NIL_XALT_CONCAT(nil_gate_port_eq_, V),                                \
    .destroy = NIL_XALT_CONCAT(nil_gate_port_destroy_, V)                       \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NO_PORT_INFOS() (struct nil_gate_port_infos){                  \
    .infos = NULL,                                                              \
    .size = 0                                                                   \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_PORT_INFOS(...) (struct nil_gate_port_infos){                  \
    .infos = (struct nil_gate_port_info[]){                                     \
        NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))      \
        (NIL_GATE_PORT_INFO, __VA_ARGS__)                                       \
    },                                                                          \
    .size = NIL_XALT_NARG(NULL, __VA_ARGS__)                                    \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NO_RPORTS(...) (struct nil_gate_rports) {                      \
    .ports = NULL,                                                              \
    .size = 0                                                                   \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_RPORTS(...) (struct nil_gate_rports) {                         \
    .ports = (struct nil_gate_rport[]){                                         \
        NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))      \
        (nil_gate_to_rport, __VA_ARGS__)                                        \
    },                                                                          \
    .size = NIL_XALT_NARG(NULL, __VA_ARGS__)                                    \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NO_MPORTS(...) (struct nil_gate_mports) {                      \
    .ports = NULL,                                                              \
    .size = 0                                                                   \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_MPORTS(...) (struct nil_gate_mports) {                         \
    .ports = (struct nil_gate_mport[]){                                         \
        NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))      \
        (nil_gate_mport_to_mport, __VA_ARGS__)                                  \
    },                                                                          \
    .size = NIL_XALT_NARG(NULL, __VA_ARGS__)                                    \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NO_NODE_OUTPUTS() (struct nil_gate_rports){                    \
    .ports = NULL,                                                              \
    .size = 0                                                                   \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NODE_OUTPUTS(COUNT) (struct nil_gate_rports){                  \
    .ports = (struct nil_gate_rport[]){                                         \
        NIL_XALT_CONCAT(NIL_XALT_REPEAT_, COUNT)(((struct nil_gate_rport){      \
            .handle = NULL, .info = {.eq = NULL, .destroy = NULL}               \
        }))                                                                     \
    },                                                                          \
    .size = (COUNT)                                                             \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_BATCH(COUNT) (struct nil_gate_bports){                         \
    .handle = NULL,                                                             \
    .size = (COUNT),                                                            \
    .ports = (struct nil_gate_bport[]){                                         \
        NIL_XALT_CONCAT(NIL_XALT_REPEAT_, COUNT)(((struct nil_gate_bport){      \
            .handle = NULL, .info = {.eq = NULL, .destroy = NULL}               \
        }))                                                                     \
    }                                                                           \
}

    // clang-format on

#ifdef __cplusplus
}

#endif
