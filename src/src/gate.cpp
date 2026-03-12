#include <nil/gate.h>
#include <nil/gate.hpp>
#include <nil/gate/runners/Immediate.hpp>
#include <nil/gate/runners/NonBlocking.hpp>
#include <nil/gate/runners/Parallel.hpp>
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

    void nil_gate_core_set_runner_non_blocking(nil_gate_core* core)
    {
        delete core->core->get_runner();                               // NOLINT
        core->core->set_runner(new nil::gate::runners::NonBlocking()); // NOLINT
    }

    void nil_gate_core_set_runner_parallel_blocking(nil_gate_core* core, uint32_t thread_count)
    {
        delete core->core->get_runner();                                        // NOLINT
        core->core->set_runner(new nil::gate::runners::Parallel(thread_count)); // NOLINT
    }

    void nil_gate_core_unset_runner(struct nil_gate_core* core)
    {
        delete core->core->get_runner(); // NOLINT
        core->core->set_runner(nullptr);
    }

    void nil_gate_core_post(struct nil_gate_core* core, struct nil_gate_core_callable callable)
    {
        core->core->post(
            [callable](nil::gate::Graph& gate_graph)
            {
                if (callable.exec != nullptr)
                {
                    struct nil_gate_graph graph
                    {
                        .graph = &gate_graph
                    };
                    callable.exec(&graph, callable.context);
                }
                if (callable.context != nullptr && callable.destroy_context != nullptr)
                {
                    callable.destroy_context(callable.context);
                }
            }
        );
    }

    void nil_gate_core_apply(struct nil_gate_core* core, struct nil_gate_core_callable callable)
    {
        core->core->apply(
            [callable](nil::gate::Graph& gate_graph)
            {
                if (callable.exec != nullptr)
                {
                    struct nil_gate_graph graph
                    {
                        .graph = &gate_graph
                    };
                    callable.exec(&graph, callable.context);
                }
                if (callable.context != nullptr && callable.destroy_context != nullptr)
                {
                    callable.destroy_context(callable.context);
                }
            }
        );
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
        return nil_gate_eport{
            .handle = handle,
            .sub_handle = handle->to_direct(),
            .info = meta_info
        };
    }

    void nil_gate_mport_set_value(nil_gate_mport port, void* new_value)
    {
        static_cast<nil::gate::ports::Mutable<nil::gate::c::PortType>*>(port.handle)
            ->set_value(nil::gate::c::PortType(new_value, port.info.eq, port.info.destroy));
    }

    void nil_gate_eport_set_value(nil_gate_eport port, void* new_value)
    {
        static_cast<nil::gate::ports::External<nil::gate::c::PortType>*>(port.handle)
            ->to_direct()
            ->set_value(nil::gate::c::PortType(new_value, port.info.eq, port.info.destroy));
    }

    void nil_gate_mport_unset_value(nil_gate_mport port)
    {
        static_cast<nil::gate::ports::Mutable<nil::gate::c::PortType>*>(port.handle)->unset_value();
    }

    void nil_gate_eport_unset_value(nil_gate_eport port)
    {
        static_cast<nil::gate::ports::External<nil::gate::c::PortType>*>(port.handle)
            ->to_direct()
            ->unset_value();
    }

    void nil_gate_graph_node(const nil_gate_graph* graph, struct nil_gate_node_info node_info)
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

        auto ng_opt_output_info = [&node_info]()
        {
            std::vector<nil_gate_port_info> object;
            object.reserve(node_info.outputs.size);
            for (auto i = 0U; i < node_info.outputs.size; ++i)
            {
                object.push_back(node_info.outputs.infos[i]);
            }
            return object;
        }();

        auto ng_fn = [exec = node_info.exec,
                      context = node_info.context,
                      ng_opt_output_info = std::move(ng_opt_output_info)] //
            (const ng::UNode<ng::c::PortType>::Arg& arg) -> void
        {
            auto arg_inputs = [&]()
            {
                std::vector<const void*> object;
                object.reserve(arg.inputs.size());
                for (const auto& i : arg.inputs)
                {
                    object.push_back(i.get().value);
                }
                return object;
            }();

            auto arg_opt_outputs = [&]()
            {
                std::vector<nil_gate_mport> object;
                object.reserve(arg.outputs.size());
                for (auto i = 0U; i < arg.outputs.size(); ++i)
                {
                    object.push_back(
                        nil_gate_mport{.handle = arg.outputs[i], .info = ng_opt_output_info[i]}
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
                    .ports = arg_opt_outputs.data(),
                },
                .context = context
            };

            exec(&args);
        };

        auto node_result = graph->graph
                               ->unode<ng::c::PortType>(
                                   {.inputs = std::move(ng_inputs),
                                    .output_size = node_info.outputs.size,
                                    .fn = std::move(ng_fn)}
                               )
                               ->outputs();

        for (auto i = 0U; i < node_info.outputs.size; ++i)
        {
            const auto ii = i + node_info.outputs.size;
            node_info.output_handles->ports[ii].info = node_info.outputs.infos[i];
            node_info.output_handles->ports[ii].handle = node_result[i];
        }
    }

    nil_gate_rport nil_gate_mport_to_rport(nil_gate_mport port)
    {
        return {.handle = port.handle, .info = port.info};
    }

    nil_gate_rport nil_gate_rport_to_rport(nil_gate_rport port)
    {
        return port;
    }

    nil_gate_mport nil_gate_mport_to_mport(nil_gate_mport port)
    {
        return port;
    }

    nil_gate_rport nil_gate_eport_to_rport(nil_gate_eport port)
    {
        return {.handle = port.sub_handle, .info = port.info};
    }

    nil_gate_mport nil_gate_eport_to_mport(nil_gate_eport port)
    {
        return {.handle = port.sub_handle, .info = port.info};
    }
}
