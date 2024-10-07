#pragma once

#include "../traits/is_edge_type_valid.hpp"

#include <concepts>
#include <type_traits>

namespace nil::gate::detail
{
    template <typename T>
    concept is_equally_comparable = requires(T l, T r) {
        { l == r } -> std::same_as<bool>;
    };

    template <typename T>
    struct input_validate
    {
        static constexpr auto value
            = gate::traits::is_edge_type_valid_v<T> && std::is_copy_constructible_v<T>;
    };

    template <typename T>
    struct input_validate<const T>: input_validate<T>
    {
    };

    template <typename T>
    struct input_validate<const T*>: input_validate<T>
    {
    };

    template <typename T>
    struct input_validate<const T&>: input_validate<T>
    {
    };

    template <typename T>
    struct input_validate<const T&&>: input_validate<T>
    {
    };

    template <typename T>
    struct input_validate<T&>
    {
        static constexpr auto value = false;
    };

    template <typename T>
    struct input_validate<T&&>
    {
        static constexpr auto value = false;
    };

    template <typename T>
    constexpr bool input_validate_v = //
        input_validate<T>::value;

    template <typename T>
    constexpr bool sync_output_validate_v =      //
        std::is_same_v<T, std::decay_t<T>>       //
        && gate::traits::is_edge_type_valid_v<T> //
        && is_equally_comparable<T>;

    template <typename T>
    constexpr bool async_output_validate_v       //
        = std::is_same_v<T, std::decay_t<T>>     //
        && gate::traits::is_edge_type_valid_v<T> //
        && is_equally_comparable<T>;
}
