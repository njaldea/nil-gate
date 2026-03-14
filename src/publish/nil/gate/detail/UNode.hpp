#pragma once

#include "../INode.hpp"
#include "nil/gate/ports/Compatible.hpp"
#include "nil/gate/ports/Mutable.hpp"
#include "traits/node.hpp"

#include <functional>
#include <nil/xalt/checks.hpp>
#include <nil/xalt/fn_sign.hpp>

#include <utility>

namespace nil::gate
{
    class Core;

    template <typename T>
    class UNode: public INode
    {
    public:
        using base_t = UNode<T>;

        struct Arg
        {
            Core* core;
            std::vector<std::reference_wrapper<const T>> inputs;
            std::vector<ports::Mutable<T>*> outputs;
        };

        struct Info
        {
            std::vector<ports::Compatible<T>> inputs;
            std::uint64_t output_size;
            std::function<void(const Arg&)> fn;
        };

        virtual std::vector<ports::Compatible<T>>& inputs() = 0;
        virtual auto outputs() -> std::vector<ports::ReadOnly<T>*> = 0;
    };
}

namespace nil::gate::detail
{
    template <typename T>
    class UNode final: public gate::UNode<T>
    {
    public:
        UNode(Core* init_core, gate::UNode<T>::Info info)
            : core(init_core)
            , fn(info.fn)
            , input_ports(std::move(info.inputs))
            , output_ports(info.output_size)
        {
            for (auto& i : input_ports)
            {
                i.attach_out(this);
            }

            output_port_handles.reserve(info.output_size);
            moutput_ports.reserve(info.output_size);
            for (auto& o : output_ports)
            {
                o.attach_in(this);
                moutput_ports.push_back(&o);
                output_port_handles.push_back(&o);
            }

            score();
        }

        ~UNode() noexcept override = default;

        UNode(UNode&&) noexcept = delete;
        UNode& operator=(UNode&&) noexcept = delete;

        UNode(const UNode&) = delete;
        UNode& operator=(const UNode&) = delete;

        std::uint32_t score() const noexcept override
        {
            if (!current_score.has_value())
            {
                if (input_ports.empty())
                {
                    current_score = 0;
                }
                else
                {
                    auto max_score = 0U;
                    for (const auto& port : input_ports)
                    {
                        max_score = std::max(max_score, port.score());
                    }
                    current_score = max_score + 1U;
                }
            }
            return current_score.value();
        }

        void exec() override
        {
            std::vector<std::reference_wrapper<const T>> p_inputs;
            p_inputs.reserve(input_ports.size());
            for (auto& i : input_ports)
            {
                p_inputs.push_back(std::cref(i.value()));
            }

            fn({.core = core, .inputs = std::move(p_inputs), .outputs = moutput_ports});
        }

        void pend() override
        {
            if (node_state != INode::ENodeState::Pending)
            {
                node_state = INode::ENodeState::Pending;
            }
        }

        void done() override
        {
            if (node_state != INode::ENodeState::Done)
            {
                node_state = INode::ENodeState::Done;
                input_state = INode::EInputState::Stale;
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

        auto inputs() -> std::vector<ports::Compatible<T>>& override
        {
            return input_ports;
        }

        auto outputs() -> std::vector<ports::ReadOnly<T>*> override
        {
            return output_port_handles;
        }

        bool is_input_changed() const override
        {
            return input_state == INode::EInputState::Changed;
        }

        bool is_pending() const override
        {
            return node_state == INode::ENodeState::Pending;
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
            input_state = INode::EInputState::Changed;
        }

        void detach_in(IPort* port) override
        {
            bool result = false;
            for (auto& i : input_ports)
            {
                result |= i.detach(port);
            }
            if (result)
            {
                current_score = {};
            }
        }

    private:
        INode::ENodeState node_state = INode::ENodeState::Pending;
        INode::EInputState input_state = INode::EInputState::Changed;

        Core* core;
        std::function<void(const typename gate::UNode<T>::Arg&)> fn;

        std::vector<ports::Compatible<T>> input_ports;
        std::vector<detail::Port<traits::portify_t<T>>> output_ports;
        std::vector<ports::Mutable<T>*> moutput_ports;        // to be passed to the node
        std::vector<ports::ReadOnly<T>*> output_port_handles; // to be returned by the node
        mutable std::optional<std::uint32_t> current_score;
    };
}
