#include <nil/gate.h>
#include <nil/gate.hpp>
#include <nil/gate/runners/Async.hpp>
#include <nil/gate/runners/Immediate.hpp>
#include <nil/gate/runners/SoftBlocking.hpp>

#include "PortType.hpp"

extern "C"
{
    struct nil_gate_core
    {
        nil::gate::Core* core;
    };

    struct nil_gate_graph
    {
        nil::gate::Graph* graph;
    };

    nil_gate_core* nil_gate_core_create(void)
    {
        auto* core = new nil_gate_core();   // NOLINT
        core->core = new nil::gate::Core(); // NOLINT
        return core;
    }

    void nil_gate_core_destroy(nil_gate_core* core)
    {
        delete core->core; // NOLINT
        delete core;       // NOLINT
    }

    void nil_gate_core_commit(const nil_gate_core* core)
    {
        core->core->commit();
    }

    void nil_gate_core_set_runner_immediate(nil_gate_core* core)
    {
        delete core->core->get_runner();                             // NOLINT
        core->core->set_runner(new nil::gate::runners::Immediate()); // NOLINT
    }

    void nil_gate_core_set_runner_soft_blocking(nil_gate_core* core)
    {
        delete core->core->get_runner();                                // NOLINT
        core->core->set_runner(new nil::gate::runners::SoftBlocking()); // NOLINT
    }

    void nil_gate_core_set_runner_async(nil_gate_core* core, uint32_t thread_count)
    {
        delete core->core->get_runner();                                     // NOLINT
        core->core->set_runner(new nil::gate::runners::Async(thread_count)); // NOLINT
    }

    void nil_gate_core_unset_runner(nil_gate_core* core)
    {
        delete core->core->get_runner(); // NOLINT
        core->core->set_runner(nullptr);
    }

    struct nil_gate_raii_context
    {
        explicit nil_gate_raii_context(void* init_context, void (*init_cleanup)(void*))
            : context(init_context)
            , cleanup(init_cleanup)
        {
        }

        nil_gate_raii_context(nil_gate_raii_context&& o) noexcept = delete;
        nil_gate_raii_context(const nil_gate_raii_context& o) = delete;

        nil_gate_raii_context& operator=(const nil_gate_raii_context& o) = delete;
        nil_gate_raii_context& operator=(nil_gate_raii_context&& o) = delete;

        ~nil_gate_raii_context()
        {
            if (cleanup != nullptr)
            {
                cleanup(context);
            }
        }

        void* context;
        void (*cleanup)(void*);
    };

    void nil_gate_core_post(nil_gate_core* core, nil_gate_core_callable callable)
    {
        auto scoped = std::make_shared<nil_gate_raii_context>(callable.context, callable.cleanup);
        core->core->post(
            [exec = callable.exec, scoped = std::move(scoped)](nil::gate::Graph& gate_graph) mutable
            {
                if (exec != nullptr)
                {
                    nil_gate_graph graph{.graph = &gate_graph};
                    exec(&graph, scoped->context);
                }
            }
        );
    }

    void nil_gate_core_apply(nil_gate_core* core, nil_gate_core_callable callable)
    {
        nil_gate_core_post(core, callable);
        nil_gate_core_commit(core);
    }

    nil_gate_eport nil_gate_graph_port(
        nil_gate_graph* graph,
        nil_gate_port_info meta_info,
        void* initial_value
    )
    {
        auto* handle = graph->graph->port(
            nil::gate::c::PortType(initial_value, meta_info.eq, meta_info.destroy)
        );
        return nil_gate_eport{.handle = handle, .info = meta_info};
    }

    const void* nil_gate_rport_value(nil_gate_rport port)
    {
        return static_cast<nil::gate::ports::ReadOnly<nil::gate::c::PortType>*>(port.handle)
            ->value()
            .value;
    }

    int nil_gate_rport_has_value(nil_gate_rport port)
    {
        return static_cast<nil::gate::ports::ReadOnly<nil::gate::c::PortType>*>(port.handle)
                   ->has_value()
            ? 1
            : 0;
    }

    void nil_gate_mport_set_value(nil_gate_mport port, void* new_value)
    {
        static_cast<nil::gate::ports::Mutable<nil::gate::c::PortType>*>(port.handle)
            ->set_value(nil::gate::c::PortType(new_value, port.info.eq, port.info.destroy));
    }

    void nil_gate_mport_unset_value(nil_gate_mport port)
    {
        static_cast<nil::gate::ports::Mutable<nil::gate::c::PortType>*>(port.handle)->unset_value();
    }

    nil_gate_node nil_gate_graph_node(const nil_gate_graph* graph, nil_gate_node_info node_info)
    {
        namespace ng = nil::gate;
        auto ng_inputs = [&node_info]()
        {
            std::vector<ng::ports::Compatible<ng::c::PortType>> object;
            object.reserve(node_info.inputs.size);
            for (auto i = 0U; i < node_info.inputs.size; ++i)
            {
                object.emplace_back(static_cast<ng::ports::ReadOnly<ng::c::PortType>*>(
                    node_info.inputs.ports[i].handle
                ));
            }
            return object;
        }();

        auto ng_output_info = [&node_info]()
        {
            std::vector<nil_gate_port_info> object;
            object.reserve(node_info.outputs.size);
            for (auto i = 0U; i < node_info.outputs.size; ++i)
            {
                object.push_back(node_info.outputs.infos[i]);
            }
            return object;
        }();

        auto scoped = std::make_shared<nil_gate_raii_context>(node_info.context, node_info.cleanup);
        auto ng_fn = [exec = node_info.exec,
                      scoped = std::move(scoped),
                      ng_output_info = std::move(ng_output_info)] //
            (const ng::UNode<ng::c::PortType>::Arg& arg) -> void
        {
            auto arg_inputs = [&]()
            {
                std::vector<const void*> object;
                object.reserve(arg.inputs.size());
                for (const auto& i : arg.inputs)
                {
                    object.push_back(i->value);
                }
                return object;
            }();

            auto arg_outputs = [&]()
            {
                std::vector<nil_gate_mport> object;
                object.reserve(arg.outputs.size());
                for (auto i = 0U; i < arg.outputs.size(); ++i)
                {
                    object.push_back(
                        nil_gate_mport{.handle = arg.outputs[i], .info = ng_output_info[i]}
                    );
                }
                return object;
            }();

            auto ngc = nil_gate_core{.core = arg.core};

            auto args = nil_gate_node_args{
                .core = &ngc,
                .inputs = {
                    .size = static_cast<std::uint8_t>(arg.inputs.size()),
                    .data=arg_inputs.data()
                },
                .outputs = {
                    .size = static_cast<std::uint8_t>(arg.outputs.size()),
                    .ports = arg_outputs.data(),
                },
                .context = scoped->context
            };

            exec(&args);
        };

        return {
            .handle = graph->graph->unode<ng::c::PortType>(
                {.inputs = std::move(ng_inputs),
                 .output_size = node_info.outputs.size,
                 .fn = std::move(ng_fn)}
            )
        };
    }

    uint8_t nil_gate_node_output_size(nil_gate_node node)
    {
        return uint8_t(
            static_cast<nil::gate::UNode<nil::gate::c::PortType>*>(node.handle)->outputs().size()
        );
    }

    void nil_gate_node_outputs(nil_gate_node node, nil_gate_rport* outputs)
    {
        const auto& node_outputs
            = static_cast<nil::gate::UNode<nil::gate::c::PortType>*>(node.handle)->outputs();
        for (auto i = 0U; i < node_outputs.size(); ++i)
        {
            outputs[i].handle = node_outputs[i];
            // port always has a "value"
            // has_value only returns true if he value of PortType is not null
            // but the port always has a PortType
            outputs[i].info.eq = node_outputs[i]->value().eq;
            outputs[i].info.destroy = node_outputs[i]->value().destroy;
        }
    }

    nil_gate_rport nil_gate_mport_as_input(nil_gate_mport port)
    {
        return {.handle = port.handle, .info = port.info};
    }

    nil_gate_rport nil_gate_rport_as_input(nil_gate_rport port)
    {
        return port;
    }

    nil_gate_rport nil_gate_eport_as_input(nil_gate_eport port)
    {
        return nil_gate_mport_as_input(nil_gate_eport_to_direct(port));
    }

    nil_gate_mport nil_gate_eport_to_direct(nil_gate_eport port)
    {
        auto* handle
            = static_cast<nil::gate::ports::External<nil::gate::c::PortType>*>(port.handle);
        return {.handle = handle->to_direct(), .info = port.info};
    }
}
