#pragma once

#include "fn_call.hpp"

#include <memory>

namespace nil::xalt
{
    template <typename A, typename... T>
    concept fn_is_maker = requires() { A(std::declval<xalt::implicit_cast<T>>()...); };

    template <typename A>
    struct fn_strategy final
    {
        template <typename... T>
        static constexpr auto check = fn_is_maker<A, T...>;

        template <typename... T>
        static auto call(T&&... args)
        {
            return A(std::forward<T>(args)...);
        }
    };

    template <typename A>
    struct fn_strategy<std::unique_ptr<A>> final
    {
        template <typename... T>
        static constexpr auto check = fn_is_maker<A, T...>;

        template <typename... T>
        static auto call(T&&... args)
        {
            return std::make_unique<A>(std::forward<T>(args)...);
        }
    };

    template <typename A>
    struct fn_strategy<std::shared_ptr<A>> final
    {
        template <typename... T>
        static constexpr auto check = fn_is_maker<A, T...>;

        template <typename... T>
        static auto call(T&&... args)
        {
            return std::make_shared<A>(std::forward<T>(args)...);
        }
    };

    template <typename A, typename... T>
        requires(!detail::fn_impl_found<fn_strategy<A>, T && ...>)
    auto fn_make(T&&... args) = delete;

    template <typename A, typename... T>
    auto fn_make(T&&... args)
    {
        return detail::fn_impl<fn_strategy<A>, T&&...>::call(std::forward<T>(args)...);
    }

    template <typename A, typename... T>
    auto fn_make_unique(T&&... args)
    {
        return fn_make<std::unique_ptr<A>>(std::forward<T>(args)...);
    }

    template <typename A, typename... T>
    auto fn_make_shared(T&&... args)
    {
        return fn_make<std::shared_ptr<A>>(std::forward<T>(args)...);
    }
}
