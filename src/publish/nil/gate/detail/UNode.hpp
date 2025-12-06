#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "nil/gate/ports/Compatible.hpp"
#include "nil/gate/ports/Mutable.hpp"
#include "traits/node.hpp"

#include <functional>
#include <nil/xalt/checks.hpp>
#include <nil/xalt/fn_sign.hpp>

#include <stdexcept>
#include <utility>

namespace nil::gate
{
    class Core;
}

namespace nil::gate::detail
{
    template <typename T>
    class UNode final: public INode
    {
    public:
        struct Arg
        {
            const Core* core;
            std::vector<std::reference_wrapper<const T>> inputs;
            std::vector<ports::Mutable<T>*> opt_outputs;
            std::uint64_t req_outputs;
        };

        struct Info
        {
            std::vector<ports::Compatible<T>> inputs;
            std::uint64_t req_output_size;
            std::uint64_t opt_output_size;
            std::function<std::vector<T>(const Arg&)> fn;
        };

        UNode(Core* init_core, Diffs* init_diffs, Info info)
            : core(init_core)
            , fn(info.fn)
            , input_ports(std::move(info.inputs))
            , req_output_ports(info.req_output_size)
            , opt_output_ports(info.opt_output_size)
        {
            for (auto& i : input_ports)
            {
                i.attach_out(this);
            }

            output_ports.reserve(info.req_output_size + info.req_output_size);
            for (auto& o : req_output_ports)
            {
                o.attach_in(this);
                o.attach(init_diffs);
                output_ports.push_back(&o);
            }

            mopt_output_ports.reserve(info.opt_output_size);
            for (auto& o : opt_output_ports)
            {
                o.attach_in(this);
                o.attach(init_diffs);
                output_ports.push_back(&o);
                mopt_output_ports.push_back(&o);
            }
        }

        ~UNode() noexcept override
        {
            for (auto& i : input_ports)
            {
                i.detach_out(this);
            }
        }

        UNode(UNode&&) noexcept = delete;
        UNode& operator=(UNode&&) noexcept = delete;

        UNode(const UNode&) = delete;
        UNode& operator=(const UNode&) = delete;

        int score() const noexcept override
        {
            if (input_ports.empty())
            {
                return 0;
            }
            int max_score = 0;
            for (const auto& port : input_ports)
            {
                max_score = std::max(max_score, port.score());
            }
            return max_score + 1;
        }

        void exec() override
        {
            std::vector<std::reference_wrapper<const T>> p_inputs;
            p_inputs.reserve(input_ports.size());
            for (auto& i : input_ports)
            {
                p_inputs.push_back(std::cref(i.value()));
            }

            auto result = fn(
                {.core = core,
                 .inputs = std::move(p_inputs),
                 .opt_outputs = mopt_output_ports,
                 .req_outputs = req_output_ports.size()}
            );
            if (result.size() != req_output_ports.size())
            {
                throw std::runtime_error("returned data by node does not match");
            }
            for (auto i = 0U; i < req_output_ports.size(); ++i)
            {
                req_output_ports[i].set(std::move(result[i]));
            }
        }

        void pend() override
        {
            if (node_state != ENodeState::Pending)
            {
                node_state = ENodeState::Pending;
                for (auto& outs : req_output_ports)
                {
                    outs.pend();
                }
            }
        }

        void done() override
        {
            if (node_state != ENodeState::Done)
            {
                node_state = ENodeState::Done;
                input_state = EInputState::Stale;
                for (auto& outs : req_output_ports)
                {
                    outs.done();
                }
            }
        }

        void run() override
        {
            if (is_pending() && is_ready())
            {
                if (is_input_changed())
                {
                    exec();
                }
                done();
            }
        }

        auto outputs()
        {
            return output_ports;
        }

        bool is_input_changed() const override
        {
            return input_state == EInputState::Changed;
        }

        bool is_pending() const override
        {
            return node_state == ENodeState::Pending;
        }

        bool is_ready() const override
        {
            for (const auto& i : input_ports)
            {
                if (!i.is_ready())
                {
                    return false;
                }
            }
            return true;
        }

        void input_changed() override
        {
            input_state = EInputState::Changed;
        }

        void detach_in(IPort* port) override
        {
            for (auto& i : input_ports)
            {
                i.detach(port);
            }
        }

    private:
        ENodeState node_state = ENodeState::Pending;
        EInputState input_state = EInputState::Changed;

        const Core* core;
        std::function<std::vector<T>(const Arg&)> fn;

        std::vector<ports::Compatible<T>> input_ports;
        std::vector<detail::Port<traits::portify_t<T>>> req_output_ports;
        std::vector<detail::Port<traits::portify_t<T>>> opt_output_ports;
        std::vector<ports::Mutable<T>*> mopt_output_ports;
        std::vector<ports::ReadOnly<T>*> output_ports;
    };
}

namespace nil::gate
{
    template <typename T>
    using UNode = detail::UNode<T>;
}
