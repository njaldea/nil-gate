#pragma once

#include "cast.hpp"
#include "fn_sign.hpp"
#include "tlist.hpp"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace nil::xalt::detail
{
    template <std::size_t value>
    consteval auto make_mask() -> std::size_t
    {
        return value == 0 ? 0 : (std::size_t(1) << (value)) - 1;
    }

    template <typename L, typename R>
    struct not_hit;

    template <auto V, auto M>
    struct not_hit<typify<V>, typify<M>> final
    {
        static constexpr auto value = ((std::size_t(1) << V) & M) == 0;
    };

    template <typename L, typename R>
    struct inverse;

    template <auto V, auto M>
    struct inverse<typify<V>, typify<M>> final
    {
        using type = typify<M - 1 - V>;
    };

    struct invalid final
    {
    };

    template <typename A, typename... Args>
    struct fn_impl final
    {
    public:
        static auto call(Args... args)
        {
            return apply<detail::make_mask<sizeof...(Args)>()>(std::forward<Args>(args)...);
        }

    private:
        template <std::size_t M>
        static auto apply(Args... args)
        {
            using masked_type //
                = typename xalt::to_tlist_t<std::make_index_sequence<sizeof...(Args)>>::
                    template apply_t<inverse, typify<sizeof...(Args)>>::
                        template remove_if<not_hit, typify<M>>;

            if constexpr (check(masked_type()))
            {
                return call(masked_type(), std::forward<Args>(args)...);
            }
            else if constexpr (M > 0)
            {
                return apply<M - 1>(std::forward<Args>(args)...);
            }
            else
            {
                return invalid();
            }
        }

        template <std::size_t... I>
        static constexpr auto check(tlist<typify<I>...> /* i */)
        {
            using arg_list = tlist<Args...>;
            return A::template check<typename arg_list::template at<sizeof...(Args) - I - 1>...>;
        }

        template <std::size_t... I>
        static auto call(tlist<typify<I>...> /* i */, Args... args)
        {
            const auto t = std::make_tuple(explicit_cast<Args>{&args}...);
            return A::call(std::get<sizeof...(Args) - I - 1>(t).cast()...);
        }
    };

    template <typename A, typename... Args>
        requires(sizeof...(Args) >= 10)
    struct fn_impl<A, Args...> final
    {
        static_assert(sizeof...(Args) < 10); // only use with less than 10 arguments
        static auto call(Args... args) = delete;
    };

    template <typename A, typename... Args>
    concept fn_impl_found
        = !std::is_same_v<invalid, decltype(fn_impl<A, Args...>::call(std::declval<Args>()...))>;
}

namespace nil::xalt
{
    template <typename A>
    struct fn_strategy;

    template <typename A>
    struct fn_strategy<fn_sign<A>>
    {
        template <typename S, typename... T>
        static constexpr auto check
            = requires(S s) { s(std::declval<xalt::implicit_cast<T>>()...); };

        template <typename... T>
        static auto call(A&& callable, T&&... args)
        {
            return callable(std::forward<T>(args)...);
        }
    };

    template <typename A, typename... T>
    auto fn_call(A&& callable, T&&... args)
    {
        return detail::fn_impl<fn_strategy<fn_sign<A>>, A&&, T&&...>::call(
            std::forward<A>(callable),
            std::forward<T>(args)...
        );
    }

    template <auto A, typename... T>
        requires(!is_fn<decltype(A)>)
    auto fn_call(T&&... args) = delete;
}
