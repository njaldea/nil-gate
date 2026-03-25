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
    struct nil_gate_core;

    struct nil_gate_core* nil_gate_core_create(void);
    void nil_gate_core_destroy(struct nil_gate_core* core);
    void nil_gate_core_commit(struct nil_gate_core* core);

    void nil_gate_core_set_runner_immediate(struct nil_gate_core* core);
    void nil_gate_core_set_runner_soft_blocking(struct nil_gate_core* core);
    void nil_gate_core_set_runner_async(struct nil_gate_core* core, uint32_t thread_count);
    void nil_gate_core_unset_runner(struct nil_gate_core* core);

    struct nil_gate_graph;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_core_callable
    {
        void (*exec)(struct nil_gate_graph*, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_gate_core_callable;

    void nil_gate_core_post(struct nil_gate_core* core, nil_gate_core_callable callable);
    void nil_gate_core_apply(struct nil_gate_core* core, nil_gate_core_callable callable);

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_port_info
    {
        int (*eq)(const void*, const void*);
        void (*destroy)(void*);
    } nil_gate_port_info;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_eport
    {
        void* handle; // external_port
        nil_gate_port_info info;
    } nil_gate_eport;

    struct nil_gate_eport nil_gate_graph_port(
        struct nil_gate_graph* graph,
        nil_gate_port_info info,
        void* initial_value
    );

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_port_infos
    {
        uint8_t size;
        const nil_gate_port_info* infos;
    } nil_gate_port_infos;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_rport
    {
        void* handle; // readonly_port
        nil_gate_port_info info;
    } nil_gate_rport;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_rports
    {
        uint8_t size;
        const nil_gate_rport* ports;
    } nil_gate_rports;

    // node execution

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_mport
    {
        void* handle; // mutable_port
        nil_gate_port_info info;
    } nil_gate_mport;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_mports
    {
        uint8_t size;
        const nil_gate_mport* ports;
    } nil_gate_mports;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_node_args_inputs
    {
        uint8_t size;
        const void* const* data;
    } nil_gate_node_args_inputs;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_node_args
    {
        struct nil_gate_core* core;
        nil_gate_node_args_inputs inputs;
        nil_gate_mports outputs;
        void* context;
    } nil_gate_node_args;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_node_info
    {
        void (*exec)(const nil_gate_node_args* args);
        nil_gate_rports inputs;
        nil_gate_port_infos outputs;
        void* context;
        void (*cleanup)(void*); // to be stored in node_handle and be called with remove
    } nil_gate_node_info;

    // NOLINTNEXTLINE(modernize-use-using)
    typedef struct nil_gate_node
    {
        void* handle;
    } nil_gate_node;

    nil_gate_node nil_gate_graph_node(
        struct nil_gate_graph const* graph,
        nil_gate_node_info node_info
    );

    uint8_t nil_gate_node_output_size(nil_gate_node node);
    void nil_gate_node_outputs(nil_gate_node node, nil_gate_rport* outputs);

    const void* nil_gate_rport_value(nil_gate_rport port);
    int nil_gate_rport_has_value(nil_gate_rport port);

    void nil_gate_mport_set_value(nil_gate_mport port, void* new_value);
    void nil_gate_mport_unset_value(nil_gate_mport port);

    nil_gate_rport nil_gate_mport_as_input(nil_gate_mport port);
    nil_gate_rport nil_gate_rport_as_input(nil_gate_rport port);
    nil_gate_rport nil_gate_eport_as_input(nil_gate_eport port);

    nil_gate_mport nil_gate_eport_to_direct(nil_gate_eport port);

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define nil_gate_port_as_input(V)                                                                                                             \
    _Generic((V), nil_gate_mport: nil_gate_mport_as_input, nil_gate_eport: nil_gate_eport_as_input, nil_gate_rport: nil_gate_rport_as_input)( \
        V                                                                                                                                     \
    )

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_PORT_INFO(V)                                                                      \
    (nil_gate_port_info)                                                                           \
    {                                                                                              \
        .eq = NIL_XALT_CONCAT(nil_gate_port_eq_, V),                                               \
        .destroy = NIL_XALT_CONCAT(nil_gate_port_destroy_, V)                                      \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NO_PORT_INFOS()                                                                   \
    (nil_gate_port_infos)                                                                          \
    {                                                                                              \
        .size = 0, .infos = NULL                                                                   \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_PORT_INFOS(...)                                                                   \
    (nil_gate_port_infos)                                                                          \
    {                                                                                              \
        .size = NIL_XALT_NARG(NULL, __VA_ARGS__), .infos = (nil_gate_port_info[])                  \
        {                                                                                          \
            NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))                     \
            (NIL_GATE_PORT_INFO, __VA_ARGS__)                                                      \
        }                                                                                          \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_NO_INPUT_PORTS()                                                                  \
    (nil_gate_rports)                                                                              \
    {                                                                                              \
        .size = 0, .ports = NULL                                                                   \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NIL_GATE_INPUT_PORTS(...)                                                                  \
    (nil_gate_rports)                                                                              \
    {                                                                                              \
        .size = NIL_XALT_NARG(NULL, __VA_ARGS__), .ports = (nil_gate_rport[])                      \
        {                                                                                          \
            NIL_XALT_CONCAT(NIL_XALT_APPLY_, NIL_XALT_NARG(NULL, __VA_ARGS__))                     \
            (nil_gate_port_as_input, __VA_ARGS__)                                                  \
        }                                                                                          \
    }

#ifdef __cplusplus
}

#endif
