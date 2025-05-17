#pragma once

#include "../Core.hpp"

#include <nil/xalt/checks.hpp>
#include <nil/xalt/tlist.hpp>

#include <utility>

namespace nil::gate::nodes
{
    namespace detail
    {
        template <
            typename T,
            bool has_core,
            bool has_async,
            typename ReturnType,
            typename Inputs,
            typename SyncOutput,
            typename ASyncOutput>
        struct AsyncedCRTP;

        template <
            typename T,
            bool has_core,
            bool has_async,
            typename ReturnType,
            typename... I,
            typename... S,
            typename... A>
        struct AsyncedCRTP<
            T,
            has_core,
            has_async,
            ReturnType,
            xalt::tlist<I...>,
            xalt::tlist<S...>,
            xalt::tlist<A...>>
        {
            void operator()(
                const Core& core,
                const nil::gate::async_outputs<S..., A...>& asyncs,
                I... args
            )
            {
                if constexpr (sizeof...(S) == 0)
                {
                    call(asyncs, std::index_sequence_for<A...>(), core, std::forward<I>(args)...);
                }
                else if constexpr (xalt::is_of_template_v<ReturnType, std::tuple>)
                {
                    auto res = call(
                        asyncs,
                        std::index_sequence_for<A...>(),
                        core,
                        std::forward<I>(args)...
                    );

                    auto batch = core.batch(asyncs);
                    [&]<std::size_t... indices>() {
                        (..., get<indices>(batch)->set_value(get<indices>(res)));
                    }(std::index_sequence_for<S...>());
                }
                else
                {
                    auto res = call(
                        asyncs,
                        std::index_sequence_for<A...>(),
                        core,
                        std::forward<I>(args)...
                    );

                    auto batch = core.batch(asyncs);
                    get<0>(batch)->set_value(std::move(res));
                }
            }

            template <std::size_t... indices>
            auto call(
                auto& asyncs,
                std::index_sequence<indices...> /* unused */,
                const Core& core,
                I... args
            )
            {
                auto* self = static_cast<T*>(this);
                if constexpr (!has_core && !has_async)
                {
                    return std::invoke(self->node, std::forward<I>(args)...);
                }
                else if constexpr (has_core && !has_async)
                {
                    return std::invoke(self->node, core, std::forward<I>(args)...);
                }
                else if constexpr (!has_core && has_async)
                {
                    return std::invoke(
                        self->node,
                        std::make_tuple(get<indices + sizeof...(S)>(asyncs)...),
                        std::forward<I>(args)...
                    );
                }
                else
                {
                    return std::invoke(
                        self->node,
                        core,
                        std::make_tuple(get<indices + sizeof...(S)>(asyncs)...),
                        std::forward<I>(args)...
                    );
                }
            }
        };
    }

    template <typename T>
    struct Asynced
        : detail::AsyncedCRTP<
              Asynced<T>,
              gate::detail::traits::node<T>::has_core,
              gate::detail::traits::node<T>::has_async,
              typename xalt::fn_sign<T>::return_type,
              typename gate::detail::traits::node<T>::inputs::types,
              typename gate::detail::traits::node<T>::sync_outputs::types,
              typename gate::detail::traits::node<T>::async_outputs::types>
    {
        explicit Asynced(T init_node)
            : node(std::move(init_node))
        {
        }

        using detail::AsyncedCRTP<
            Asynced<T>,
            gate::detail::traits::node<T>::has_core,
            gate::detail::traits::node<T>::has_async,
            typename xalt::fn_sign<T>::return_type,
            typename gate::detail::traits::node<T>::inputs::types,
            typename gate::detail::traits::node<T>::sync_outputs::types,
            typename gate::detail::traits::node<T>::async_outputs::types>::operator();

        T node;
    };
}
