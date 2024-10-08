#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "traits/node.hpp"

#include <memory>

namespace nil::gate
{
    class Core;
}

namespace nil::gate::detail
{
    template <typename T>
    class Node final: public INode
    {
        using input_t = typename detail::traits::node<T>::inputs;
        using sync_output_t = typename detail::traits::node<T>::sync_outputs;
        using async_output_t = typename detail::traits::node<T>::async_outputs;
        using output_t = typename detail::traits::node<T>::outputs;

    public:
        Node(Diffs* init_diffs, T init_instance, typename input_t::edges init_inputs)
            : instance(std::move(init_instance))
            , inputs(std::move(init_inputs))
        {
            std::apply([this](auto&... i) { (i.attach(this), ...); }, inputs);
            std::apply([init_diffs](auto&... o) { (o.attach(init_diffs), ...); }, sync_outputs);
            std::apply([init_diffs](auto&... o) { (o.attach(init_diffs), ...); }, async_outputs);
        }

        ~Node() noexcept override = default;

        Node(Node&&) noexcept = delete;
        Node& operator=(Node&&) noexcept = delete;

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;

        void exec(Core* core) override
        {
            if constexpr (traits::callable<T>::tag == traits::EReturnType::Void)
            {
                call(core, typename input_t::make_index_sequence());
            }
            if constexpr (traits::callable<T>::tag == traits::EReturnType::Tuple)
            {
                forward_to_output(
                    call(core, typename input_t::make_index_sequence()),
                    typename sync_output_t::make_index_sequence()
                );
            }
            if constexpr (traits::callable<T>::tag == traits::EReturnType::Mono)
            {
                auto result = call(core, typename input_t::make_index_sequence());
                if constexpr (sync_output_t::size == 1)
                {
                    auto& o = get<0>(sync_outputs);
                    if (!o.is_equal(result))
                    {
                        o.exec(std::move(result));
                    }
                }
            }
        }

        void pend() override
        {
            if (node_state != ENodeState::Pending)
            {
                node_state = ENodeState::Pending;
                std::apply([](auto&... syncs) { (syncs.pend(), ...); }, sync_outputs);
            }
        }

        void done() override
        {
            if (node_state != ENodeState::Done)
            {
                node_state = ENodeState::Done;
                input_state = EInputState::Stale;
                std::apply([](auto&... syncs) { (syncs.done(), ...); }, sync_outputs);
            }
        }

        void run(Core* core) override
        {
            if (is_pending() && is_ready())
            {
                if (input_state == EInputState::Changed)
                {
                    exec(core);
                }
                done();
            }
        }

        auto output_edges()
        {
            if constexpr (output_t::size > 0)
            {
                return typename output_t::edges(
                    std::apply(
                        [](auto&... s)
                        { return typename sync_output_t::edges(std::addressof(s)...); },
                        sync_outputs
                    ),
                    std::apply(
                        [](auto&... s)
                        { return typename async_output_t::edges(std::addressof(s)...); },
                        async_outputs
                    )
                );
            }
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
            return std::apply(
                [](const auto&... i) { return (true && ... && i.is_ready()); },
                inputs
            );
        }

        void input_changed() override
        {
            input_state = EInputState::Changed;
        }

    private:
        template <std::size_t... s_indices>
        void forward_to_output(
            [[maybe_unused]] auto result,
            std::index_sequence<s_indices...> /* unused */
        )
        {
            [[maybe_unused]] constexpr auto setter = [](auto& o, auto&& v)
            {
                if (!o.is_equal(v))
                {
                    o.exec(std::forward<decltype(v)>(v));
                }
            };
            (setter(get<s_indices>(sync_outputs), std::move(get<s_indices>(result))), ...);
        }

        template <std::size_t... i_indices>
        auto call(Core* core, std::index_sequence<i_indices...> /* unused */)
        {
            if constexpr (traits::node<T>::has_async)
            {
                if constexpr (traits::node<T>::has_core)
                {
                    return instance(
                        *core,
                        std::apply(
                            [](auto&... a)
                            { return typename async_output_t::edges(std::addressof(a)...); },
                            async_outputs
                        ),
                        get<i_indices>(inputs).value()...
                    );
                }
                else
                {
                    return instance(
                        std::apply(
                            [](auto&... a)
                            { return typename async_output_t::edges(std::addressof(a)...); },
                            async_outputs
                        ),
                        get<i_indices>(inputs).value()...
                    );
                }
            }
            else
            {
                if constexpr (traits::node<T>::has_core)
                {
                    return instance(*core, get<i_indices>(inputs).value()...);
                }
                else
                {
                    return instance(get<i_indices>(inputs).value()...);
                }
            }
        }

        ENodeState node_state = ENodeState::Pending;
        EInputState input_state = EInputState::Stale;

        T instance;

        typename input_t::edges inputs;
        typename sync_output_t::data_edges sync_outputs;
        typename async_output_t::data_edges async_outputs;
    };
}
