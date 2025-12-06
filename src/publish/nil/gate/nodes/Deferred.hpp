#pragma once

#include "../Core.hpp"

#include <nil/xalt/checks.hpp>
#include <nil/xalt/tlist.hpp>

#include <utility>

namespace nil::gate::nodes
{
    template <typename T>
    struct Deferred;

    namespace detail
    {
        template <typename T, typename Inputs, typename ReqOutput, typename OptOutput>
        struct DeferredCRTP;

        template <typename T, typename... I, typename... F, typename... O>
        struct DeferredCRTP<T, xalt::tlist<I...>, xalt::tlist<F...>, xalt::tlist<O...>>
        {
            explicit DeferredCRTP(T init_node)
                : node(std::move(init_node))
            {
            }

            void operator()(
                Core& core,
                const nil::gate::opt_outputs<F..., O...>& opts,
                const I&... args
            )
            {
                using ReturnType = typename xalt::fn_sign<T>::return_type;
                if constexpr (sizeof...(F) == 0)
                {
                    call(opts, std::index_sequence_for<O...>(), core, args...);
                }
                else if constexpr (xalt::is_of_template_v<ReturnType, std::tuple>)
                {
                    auto res = call(opts, std::index_sequence_for<O...>(), core, args...);

                    core.post(
                        [opts, res]() mutable
                        {
                            [&]<std::size_t... indices>() {
                                (..., get<indices>(opts)->set_value(std::move(get<indices>(res))));
                            }(std::index_sequence_for<F...>());
                        }
                    );
                }
                else
                {
                    auto res = call(opts, std::index_sequence_for<O...>(), core, args...);

                    core.post([opts, res]() mutable { get<0>(opts)->set_value(std::move(res)); });
                }
            }

            template <std::size_t... indices>
            auto call(
                const nil::gate::opt_outputs<F..., O...>& opts,
                std::index_sequence<indices...> /* unused */,
                const Core& core,
                const I&... args
            )
            {
                constexpr auto has_core = gate::detail::traits::node<T>::has_core;
                constexpr auto has_opt = gate::detail::traits::node<T>::has_opt;
                if constexpr (!has_core && !has_opt)
                {
                    return std::invoke(node, args...);
                }
                else if constexpr (has_core && !has_opt)
                {
                    return std::invoke(node, core, args...);
                }
                else if constexpr (!has_core && has_opt)
                {
                    return std::invoke(
                        node,
                        std::make_tuple(get<indices + sizeof...(F)>(opts)...),
                        args...
                    );
                }
                else if constexpr (has_core && has_opt)
                {
                    return std::invoke(
                        node,
                        core,
                        std::make_tuple(get<indices + sizeof...(F)>(opts)...),
                        args...
                    );
                }
            }

            T node;
        };
    }

    template <typename T>
    struct Deferred
        : detail::DeferredCRTP<
              T,
              typename gate::detail::traits::node<T>::inputs::types,
              typename gate::detail::traits::node<T>::req_outputs::types,
              typename gate::detail::traits::node<T>::opt_outputs::types>
    {
        explicit Deferred(T init_node)
            : detail::DeferredCRTP<
                  T,
                  typename gate::detail::traits::node<T>::inputs::types,
                  typename gate::detail::traits::node<T>::req_outputs::types,
                  typename gate::detail::traits::node<T>::opt_outputs::types>(std::move(init_node))
        {
        }

        using detail::DeferredCRTP<
            T,
            typename gate::detail::traits::node<T>::inputs::types,
            typename gate::detail::traits::node<T>::req_outputs::types,
            typename gate::detail::traits::node<T>::opt_outputs::types>::operator();
    };
}
