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
            , inputs(std::move(info.inputs))
            , req_outputs(info.req_output_size)
            , opt_outputs(info.opt_output_size)
        {
            for (auto& i : inputs)
            {
                i.attach(this);
            }

            outputs.reserve(info.req_output_size + info.req_output_size);
            for (auto& o : req_outputs)
            {
                o.attach(init_diffs);
                outputs.push_back(&o);
            }

            mopt_outputs.reserve(info.opt_output_size);
            for (auto& o : opt_outputs)
            {
                o.attach(init_diffs);
                outputs.push_back(&o);
                mopt_outputs.push_back(&o);
            }
        }

        ~UNode() noexcept override = default;

        UNode(UNode&&) noexcept = delete;
        UNode& operator=(UNode&&) noexcept = delete;

        UNode(const UNode&) = delete;
        UNode& operator=(const UNode&) = delete;

        void exec() override
        {
            std::vector<std::reference_wrapper<const T>> p_inputs;
            p_inputs.reserve(inputs.size());
            for (auto& i : inputs)
            {
                p_inputs.push_back(std::cref(i.value()));
            }

            auto result = fn(
                {.core = core,
                 .inputs = std::move(p_inputs),
                 .opt_outputs = mopt_outputs,
                 .req_outputs = req_outputs.size()}
            );
            if (result.size() != req_outputs.size())
            {
                throw std::runtime_error("returned data by node does not match");
            }
            for (auto i = 0U; i < req_outputs.size(); ++i)
            {
                req_outputs[i].set(std::move(result[i]));
            }
        }

        void pend() override
        {
            if (node_state != ENodeState::Pending)
            {
                node_state = ENodeState::Pending;
                for (auto& outs : req_outputs)
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
                for (auto& outs : req_outputs)
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

        auto output_ports()
        {
            return outputs;
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
            for (const auto& i : inputs)
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

    private:
        ENodeState node_state = ENodeState::Pending;
        EInputState input_state = EInputState::Changed;

        const Core* core;
        std::function<std::vector<T>(const Arg&)> fn;

        std::vector<ports::Compatible<T>> inputs;
        std::vector<detail::Port<traits::portify_t<T>>> req_outputs;
        std::vector<detail::Port<traits::portify_t<T>>> opt_outputs;
        std::vector<ports::Mutable<T>*> mopt_outputs;
        std::vector<ports::ReadOnly<T>*> outputs;
    };
}

namespace nil::gate
{
    template <typename T>
    using UNode = detail::UNode<T>;
}
