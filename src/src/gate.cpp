#include <nil/gate.h>
#include <nil/gate/runners/Immediate.hpp>
#include <nil/gate/runners/NonBlocking.hpp>
#include <nil/gate/runners/Parallel.hpp>
#include <nil/gate/runners/SoftBlocking.hpp>

#include "PortType.hpp"

extern "C"
{
    struct nil_gate_core
    {
        nil::gate::Core* handle;
    };

    nil_gate_core* nil_gate_core_create(void)
    {
        auto* core = new nil_gate_core();     // NOLINT
        core->handle = new nil::gate::Core(); // NOLINT
        return core;
    }

    void nil_gate_core_destroy(nil_gate_core* core)
    {
        delete core->handle; // NOLINT
        delete core;         // NOLINT
    }

    void nil_gate_core_commit(const nil_gate_core* core)
    {
        core->handle->commit();
    }

    void nil_gate_core_set_runner_immediate(nil_gate_core* core)
    {
        core->handle->set_runner(new nil::gate::runners::Immediate()); // NOLINT
    }

    void nil_gate_core_set_runner_soft_blocking(nil_gate_core* core)
    {
        core->handle->set_runner(new nil::gate::runners::SoftBlocking()); // NOLINT
    }

    void nil_gate_core_set_runner_non_blocking(nil_gate_core* core)
    {
        core->handle->set_runner(new nil::gate::runners::NonBlocking()); // NOLINT
    }

    void nil_gate_core_set_runner_parallel_blocking(nil_gate_core* core, uint32_t thread_count)
    {
        core->handle->set_runner(new nil::gate::runners::Parallel(thread_count)); // NOLINT
    }

    void nil_gate_core_unset_runner(struct nil_gate_core* core)
    {
        delete core->handle->get_runner(); // NOLINT
        core->handle->set_runner(nullptr);
    }

    nil_gate_mport nil_gate_core_port(
        nil_gate_core* core,
        nil_gate_port_info meta_info,
        void* initial_value
    )
    {
        return nil_gate_mport{
            .handle = core->handle->port(
                nil::gate::c::PortType(initial_value, meta_info.eq, meta_info.destroy)
            ),
            .info = meta_info
        };
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

    void nil_gate_core_node(
        const nil_gate_core* core,
        void (*fn)(struct nil_gate_node_args* args),
        struct nil_gate_rports inputs,
        struct nil_gate_port_infos req_outputs,
        struct nil_gate_port_infos opt_outputs,
        nil_gate_rports* outputs
    )
    {
        namespace ng = nil::gate;
        auto ng_inputs = [&inputs]()
        {
            std::vector<ng::ports::Compatible<ng::c::PortType>> object;
            object.reserve(inputs.size);
            for (auto i = 0U; i < inputs.size; ++i)
            {
                object.emplace_back(
                    static_cast<ng::ports::ReadOnly<ng::c::PortType>*>(inputs.ports[i].handle)
                );
            }
            return object;
        }();

        auto ng_req_output_info = [&req_outputs]()
        {
            std::vector<nil_gate_port_info> object;
            object.reserve(req_outputs.size);
            for (auto i = 0U; i < req_outputs.size; ++i)
            {
                object.push_back(req_outputs.infos[i]);
            }
            return object;
        }();

        auto ng_opt_output_info = [&opt_outputs]()
        {
            std::vector<nil_gate_port_info> object;
            object.reserve(opt_outputs.size);
            for (auto i = 0U; i < opt_outputs.size; ++i)
            {
                object.push_back(opt_outputs.infos[i]);
            }
            return object;
        }();

        auto ng_fn = [core = core,
                      fn = fn,
                      ng_req_output_info = std::move(ng_req_output_info),
                      ng_opt_output_info = std::move(ng_opt_output_info)] //
            (const ng::UNode<ng::c::PortType>::Arg& arg) -> std::vector<ng::c::PortType>
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

            std::vector<void*> arg_req_outputs(arg.req_outputs, nullptr);

            auto arg_opt_outputs = [&]()
            {
                std::vector<nil_gate_mport> object;
                object.reserve(arg.opt_outputs.size());
                for (auto i = 0U; i < arg.opt_outputs.size(); ++i)
                {
                    object.push_back(
                        nil_gate_mport{.handle = arg.opt_outputs[i], .info = ng_opt_output_info[i]}
                    );
                }
                return object;
            }();

            auto args = nil_gate_node_args{
                .core = core,
                .inputs = {
                    .size = static_cast<std::uint8_t>(arg.inputs.size()),
                    .data=arg_inputs.data()
                },
                .req_outputs = {
                    .size = static_cast<std::uint8_t>(arg.req_outputs),
                    .data = arg_req_outputs.data()
                },
                .opt_outputs = {
                    .size = static_cast<std::uint8_t>(arg.opt_outputs.size()),
                    .ports = arg_opt_outputs.data(),
                }
            };

            fn(&args);

            return [&]()
            {
                std::vector<ng::c::PortType> final_req_outputs;
                final_req_outputs.reserve(arg.req_outputs);
                for (auto i = 0U; i < arg_req_outputs.size(); ++i)
                {
                    final_req_outputs.emplace_back(
                        arg_req_outputs[i],
                        ng_req_output_info[i].eq,
                        ng_req_output_info[i].destroy
                    );
                }
                return final_req_outputs;
            }();
        };

        auto node_result = core->handle->unode<ng::c::PortType>(
            {.inputs = std::move(ng_inputs),
             .req_output_size = req_outputs.size,
             .opt_output_size = opt_outputs.size,
             .fn = std::move(ng_fn)}
        );

        for (auto i = 0U; i < req_outputs.size; ++i)
        {
            outputs->ports[i].info = req_outputs.infos[i];
            outputs->ports[i].handle = node_result[i];
        }

        for (auto i = 0U; i < opt_outputs.size; ++i)
        {
            const auto ii = i + req_outputs.size;
            outputs->ports[ii].info = opt_outputs.infos[i];
            outputs->ports[ii].handle = node_result[i];
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
}
