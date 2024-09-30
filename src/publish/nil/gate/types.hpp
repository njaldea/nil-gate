#pragma once

#include "edges/Compatible.hpp"
#include "edges/Mutable.hpp"
#include "edges/ReadOnly.hpp"

#include <tuple>
#include <type_traits>

namespace nil::gate
{
    template <typename... T>
    using inputs = std::tuple<edges::Compatible<T>...>;
    template <typename... T>
    using async_outputs = std::tuple<edges::Mutable<T>*...>;
    template <typename... T>
    using sync_outputs = std::tuple<edges::ReadOnly<T>*...>;

    template <typename S, typename A>
    struct outputs;

    template <typename... S, typename... A>
    struct outputs<sync_outputs<S...>, async_outputs<A...>>
    {
        sync_outputs<S...> syncs;
        async_outputs<A...> asyncs;
    };

    template <std::size_t I, typename... S, typename... A>
    auto get(const outputs<sync_outputs<S...>, async_outputs<A...>>& o)
    {
        if constexpr (I < sizeof...(S))
        {
            return std::get<I>(o.syncs);
        }
        else
        {
            return std::get<I - sizeof...(S)>(o.asyncs);
        }
    }
}

template <typename... S, typename... A>
struct std::tuple_size<
    nil::gate::outputs<nil::gate::sync_outputs<S...>, nil::gate::async_outputs<A...>>>
    : std::integral_constant<std::size_t, sizeof...(S) + sizeof...(A)>
{
};

template <std::size_t I, typename... S, typename... A>
struct std::tuple_element<
    I,
    nil::gate::outputs<nil::gate::sync_outputs<S...>, nil::gate::async_outputs<A...>>>
{
    using type = decltype(get<I>(               //
        std::declval<                           //
            nil::gate::outputs<                 //
                nil::gate::sync_outputs<S...>,  //
                nil::gate::async_outputs<A...>> //
            >()
    ));
};
