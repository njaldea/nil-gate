#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "traits/node.hpp"

#include <nil/xalt/checks.hpp>
#include <nil/xalt/fn_sign.hpp>

#include <memory>
#include <utility>

namespace nil::gate
{
    class Core;

    template <typename I, typename O>
    class Node;

    template <typename... I, typename... P>
    class Node<xalt::tlist<I...>, xalt::tlist<P...>>: public INode
    {
    public:
        virtual void inputs(std::tuple<ports::Compatible<traits::portify_t<std::decay_t<I>>>...>)
            = 0;
        virtual auto outputs()
            -> std::tuple<ports::ReadOnly<traits::portify_t<std::decay_t<P>>>*...> = 0;
    };
}

namespace nil::gate::detail
{
    template <typename T>
    class Node final
        : public gate::Node<
              typename detail::traits::node<T>::inputs::types,
              typename detail::traits::node<T>::outputs::types>
    {
        using node_t = detail::traits::node<T>;
        using input_t = typename node_t::inputs;
        using req_output_t = typename node_t::req_outputs;
        using opt_output_t = typename node_t::opt_outputs;
        using output_t = typename node_t::outputs;

    public:
        using base_t = gate::Node<
            typename detail::traits::node<T>::inputs::types,
            typename detail::traits::node<T>::outputs::types>;

        Node(
            Core* init_core,
            Diffs* init_diffs,
            T init_instance,
            typename input_t::ports init_inputs
        )
            : core(init_core)
            , instance(std::move(init_instance))
            , input_ports(std::move(init_inputs))
        {
            std::apply([this](auto&... i) { (i.attach_out(this), ...); }, input_ports);
            std::apply([this](auto&... o) { (o.attach_in(this), ...); }, req_outputs);
            std::apply([this](auto&... o) { (o.attach_in(this), ...); }, opt_outputs);
            std::apply([init_diffs](auto&... o) { (o.attach(init_diffs), ...); }, req_outputs);
            std::apply([init_diffs](auto&... o) { (o.attach(init_diffs), ...); }, opt_outputs);
            score();
        }

        ~Node() noexcept override
        {
            std::apply([this](auto&... i) { (i.detach_out(this), ...); }, input_ports);
        }

        Node(Node&&) noexcept = delete;
        Node& operator=(Node&&) noexcept = delete;

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;

        int score() const noexcept override
        {
            if (!current_score.has_value())
            {
                current_score = std::apply(
                    [](const auto&... ports) { return std::max<int>({ports.score()...}) + 1; },
                    input_ports
                );
            }
            return current_score.value();
        }

        void exec() override
        {
            static constexpr auto setter = []<typename U>(auto& o, U&& v)
            {
                if (!o.is_equal(v))
                {
                    o.set(std::forward<U>(v));
                }
            };
            using return_type = xalt::fn_sign<T>::return_type;
            if constexpr (std::is_same_v<return_type, void>)
            {
                call(typename input_t::make_index_sequence());
            }
            else if constexpr (xalt::is_of_template_v<return_type, std::tuple>)
            {
                auto result = call(typename input_t::make_index_sequence());
                [&]<std::size_t... i>(std::index_sequence<i...>) {
                    (setter(get<i>(req_outputs), std::move(get<i>(result))), ...);
                }(typename req_output_t::make_index_sequence());
            }
            else
            {
                setter(get<0>(req_outputs), call(typename input_t::make_index_sequence()));
            }
        }

        void pend() override
        {
            if (node_state != INode::ENodeState::Pending)
            {
                node_state = INode::ENodeState::Pending;
                std::apply([](auto&... outs) { (outs.pend(), ...); }, req_outputs);
            }
        }

        void done() override
        {
            if (node_state != INode::ENodeState::Done)
            {
                node_state = INode::ENodeState::Done;
                input_state = INode::EInputState::Stale;
                std::apply([](auto&... outs) { (outs.done(), ...); }, req_outputs);
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

        void inputs(input_t::ports ports) override
        {
            (void)ports;
        }

        typename output_t::ports outputs() override
        {
            return [&]<std::size_t... F, std::size_t... O> //
                (std::index_sequence<F...>, std::index_sequence<O...>)
            {
                return typename output_t::ports(
                    std::addressof(get<F>(req_outputs))...,
                    std::addressof(get<O>(opt_outputs))...
                );
            }(std::make_index_sequence<req_output_t::size>(),
              std::make_index_sequence<opt_output_t::size>());
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
            return std::apply(
                [](const auto&... i) { return (true && ... && i.is_ready()); },
                input_ports
            );
        }

        void input_changed() override
        {
            input_state = INode::EInputState::Changed;
        }

        void detach_in(IPort* port) override
        {
            if (std::apply(
                    [port](auto&... i)
                    {
                        bool result = false;
                        ((result |= i.detach(port)), ...);
                        return result;
                    },
                    input_ports
                ))
            {
                current_score = {};
            }
        }

    private:
        template <std::size_t... i>
        auto call(std::index_sequence<i...> /* indices */)
        {
            if constexpr (traits::node<T>::has_opt && traits::node<T>::has_core)
            {
                return instance(
                    *core,
                    std::apply(
                        [](auto&... a)
                        { return typename opt_output_t::ports(std::addressof(a)...); },
                        opt_outputs
                    ),
                    get<i>(input_ports).value()...
                );
            }
            else if constexpr (traits::node<T>::has_opt && !traits::node<T>::has_core)
            {
                return instance(
                    std::apply(
                        [](auto&... a)
                        { return typename opt_output_t::ports(std::addressof(a)...); },
                        opt_outputs
                    ),
                    get<i>(input_ports).value()...
                );
            }
            else if constexpr (!traits::node<T>::has_opt && traits::node<T>::has_core)
            {
                return instance(*core, get<i>(input_ports).value()...);
            }
            else if constexpr (!traits::node<T>::has_opt && !traits::node<T>::has_core)
            {
                return instance(get<i>(input_ports).value()...);
            }
        }

        INode::ENodeState node_state = INode::ENodeState::Pending;
        INode::EInputState input_state = INode::EInputState::Changed;

        const Core* core;
        T instance;

        typename input_t::ports input_ports;
        typename req_output_t::data_ports req_outputs;
        typename opt_output_t::data_ports opt_outputs;
        mutable std::optional<int> current_score;
    };
}
