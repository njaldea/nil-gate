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
        core->handle->set_runner<nil::gate::runners::Immediate>();
    }

    void nil_gate_core_set_runner_soft_blocking(nil_gate_core* core)
    {
        core->handle->set_runner<nil::gate::runners::SoftBlocking>();
    }

    void nil_gate_core_set_runner_non_blocking(nil_gate_core* core)
    {
        core->handle->set_runner<nil::gate::runners::NonBlocking>();
    }

    void nil_gate_core_set_runner_parallel_blocking(nil_gate_core* core, uint32_t thread_count)
    {
        core->handle->set_runner<nil::gate::runners::Parallel>(thread_count);
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

    void nil_gate_bport_set_value(struct nil_gate_bport port, void* new_value)
    {
        static_cast<nil::gate::ports::Batch<nil::gate::c::PortType>*>(port.handle)
            ->set_value(nil::gate::c::PortType(new_value, port.info.eq, port.info.destroy));
    }

    void nil_gate_bport_unset_value(struct nil_gate_bport port)
    {
        static_cast<nil::gate::ports::Batch<nil::gate::c::PortType>*>(port.handle)->unset_value();
    }

    void nil_gate_core_batch(
        struct nil_gate_core const* core,
        struct nil_gate_mports mports,
        struct nil_gate_bports* batch_ports
    )
    {
        auto ports = std::vector<nil::gate::ports::Mutable<nil::gate::c::PortType>*>();
        ports.reserve(mports.size);
        for (auto i = 0; i < batch_ports->size; ++i)
        {
            ports.emplace_back(static_cast<nil::gate::ports::Mutable<nil::gate::c::PortType>*>(
                mports.ports[i].handle
            ));
        }
        auto batch = core->handle->make_ubatch(std::move(ports));
        batch_ports->size = mports.size;
        for (auto i = 0; i < batch_ports->size; ++i)
        {
            batch_ports->ports[i].handle = (*batch)[std::size_t(i)];
            batch_ports->ports[i].info = mports.ports[i].info;
        }
        batch_ports->handle = batch.release();
    }

    void nil_gate_core_batch_release(struct nil_gate_bports* batch)
    {
        delete static_cast<nil::gate::UBatch<nil::gate::c::PortType>*>(batch->handle); // NOLINT
    }

    void nil_gate_add_node(
        const nil_gate_core* core,
        nil_gate_node_info node_info,
        nil_gate_output_ports* output_ports
    )
    {
        namespace ng = nil::gate;
        auto inputs = [node_info]()
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

        auto req_output_info = [node_info]()
        {
            std::vector<nil_gate_port_info> object;
            object.reserve(node_info.req_outputs.size);
            for (auto i = 0U; i < node_info.req_outputs.size; ++i)
            {
                object.push_back(node_info.req_outputs.infos[i]);
            }
            return object;
        }();

        auto opt_output_info = [node_info]()
        {
            std::vector<nil_gate_port_info> object;
            object.reserve(node_info.opt_outputs.size);
            for (auto i = 0U; i < node_info.opt_outputs.size; ++i)
            {
                object.push_back(node_info.opt_outputs.infos[i]);
            }
            return object;
        }();

        auto fn = [core = core,
                   fn = node_info.fn,
                   req_output_info = std::move(req_output_info),
                   opt_output_info = std::move(opt_output_info)] //
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
                        nil_gate_mport{.handle = arg.opt_outputs[i], .info = opt_output_info[i]}
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
                        req_output_info[i].eq,
                        req_output_info[i].destroy
                    );
                }
                return final_req_outputs;
            }();
        };

        auto node_result = core->handle->uniform_node<ng::c::PortType>(
            {.inputs = std::move(inputs),
             .req_output_size = node_info.req_outputs.size,
             .opt_output_size = node_info.opt_outputs.size,
             .fn = std::move(fn)}
        );

        for (auto i = 0U; i < node_info.req_outputs.size; ++i)
        {
            output_ports->ports[i].info = node_info.req_outputs.infos[i];
            output_ports->ports[i].handle = node_result[i];
        }

        for (auto i = 0U; i < node_info.opt_outputs.size; ++i)
        {
            const auto ii = i + node_info.req_outputs.size;
            output_ports->ports[ii].info = node_info.opt_outputs.infos[i];
            output_ports->ports[ii].handle = node_result[i];
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
}
