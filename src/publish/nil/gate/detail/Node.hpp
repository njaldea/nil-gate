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
        Node(
            Diffs* init_diffs,
            Core* init_core,
            typename input_t::edges init_inputs,
            T init_instance
        )
            : instance(std::move(init_instance))
            , core(init_core)
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

        void exec() override
        {
            if constexpr (traits::callable<T>::tag == traits::EReturnType::Void)
            {
                call(typename input_t::make_index_sequence());
            }
            if constexpr (traits::callable<T>::tag == traits::EReturnType::Tuple)
            {
                forward_to_output(
                    call(typename input_t::make_index_sequence()),
                    typename sync_output_t::make_index_sequence()
                );
            }
            if constexpr (traits::callable<T>::tag == traits::EReturnType::Mono)
            {
                auto result = call(typename input_t::make_index_sequence());
                if constexpr (sync_output_t::size == 1)
                {
                    get<0>(sync_outputs).exec(std::move(result));
                }
            }
        }

        void pend() override
        {
            if (state != EState::Pending)
            {
                state = EState::Pending;
                std::apply([](auto&... syncs) { (syncs.pend(), ...); }, sync_outputs);
            }
        }

        void done() override
        {
            if (state != EState::Done)
            {
                state = EState::Done;
                std::apply([](auto&... syncs) { (syncs.done(), ...); }, sync_outputs);
            }
        }

        void run() override
        {
            if (is_pending() && is_ready())
            {
                exec();
                done();
            }
        }

        auto output_edges()
        {
            if constexpr (output_t::size > 0)
            {
                return typename output_t::edges(
                    std::apply(
                        [](auto&... s) { return typename output_t::sync_t(std::addressof(s)...); },
                        sync_outputs
                    ),
                    std::apply(
                        [](auto&... s) { return typename output_t::async_t(std::addressof(s)...); },
                        async_outputs
                    )
                );
            }
        }

        bool is_pending() const override
        {
            return state == EState::Pending;
        }

        bool is_ready() const override
        {
            return std::apply(
                [](const auto&... i) { return (true && ... && i.is_ready()); },
                inputs
            );
        }

    private:
        template <std::size_t... s_indices>
        void forward_to_output(
            [[maybe_unused]] auto result,
            std::index_sequence<s_indices...> /* unused */
        )
        {
            (get<s_indices>(sync_outputs).exec(std::move(get<s_indices>(result))), ...);
        }

        template <std::size_t... i_indices>
        auto call(std::index_sequence<i_indices...> /* unused */)
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

        EState state = EState::Pending;

        T instance;
        Core* core;
        typename input_t::edges inputs;
        typename sync_output_t::data_edges sync_outputs;
        typename async_output_t::data_edges async_outputs;
    };
}
