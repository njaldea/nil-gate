#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "traits/node.hpp"

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
            const typename input_t::edges& init_inputs,
            T init_instance
        )
            : instance(std::move(init_instance))
            , core(init_core)
            , inputs(init_inputs)
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
                pend(typename sync_output_t::make_index_sequence());
            }
        }

        void done() override
        {
            if (state != EState::Done)
            {
                state = EState::Done;
                done(typename sync_output_t::make_index_sequence());
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

        typename output_t::edges output_edges()
        {
            return output_edges(
                typename sync_output_t::make_index_sequence(),
                typename async_output_t::make_index_sequence()
            );
        }

        bool is_pending() const override
        {
            return state == EState::Pending;
        }

        bool is_ready() const override
        {
            return this->are_inputs_ready(typename input_t::make_index_sequence());
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

        template <std::size_t... s_indices>
        void pend(std::index_sequence<s_indices...> /* unused */)
        {
            (get<s_indices>(sync_outputs).pend(), ...);
        }

        template <std::size_t... s_indices>
        void done(std::index_sequence<s_indices...> /* unused */)
        {
            (get<s_indices>(sync_outputs).done(), ...);
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
                        async_edges(typename async_output_t::make_index_sequence()),
                        get<i_indices>(inputs).value()...
                    );
                }
                else
                {
                    return instance(
                        async_edges(typename async_output_t::make_index_sequence()),
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

        template <std::size_t... a_indices>
        auto async_edges(std::index_sequence<a_indices...> /* unused */)
        {
            return typename async_output_t::edges( //
                std::addressof(get<a_indices>(async_outputs))...
            );
        }

        template <std::size_t... s_indices, std::size_t... a_indices>
        auto output_edges(
            std::index_sequence<s_indices...> /* unused */,
            std::index_sequence<a_indices...> /* unused */
        )
        {
            if constexpr (sizeof...(s_indices) > 0 || sizeof...(a_indices) > 0)
            {
                return typename output_t::edges(
                    {std::addressof(get<s_indices>(sync_outputs))...},
                    {std::addressof(get<a_indices>(async_outputs))...}
                );
            }
        }

        template <std::size_t... i_indices>
        bool are_inputs_ready(std::index_sequence<i_indices...> /* unused */) const
        {
            return (true && ... && std::get<i_indices>(inputs).is_ready());
        }

        EState state = EState::Pending;

        T instance;
        Core* core;
        typename input_t::edges inputs;
        typename sync_output_t::data_edges sync_outputs;
        typename async_output_t::data_edges async_outputs;
    };
}
