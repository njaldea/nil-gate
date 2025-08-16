#pragma once

#include "../traits/compare.hpp"
#include "../traits/is_port_type_valid.hpp"
#include "../traits/portify.hpp"

#include <concepts>
#include <type_traits>

namespace nil::gate::detail
{
    template <typename T>
    concept is_equally_comparable = requires(T l, T r) {
        { traits::compare<T>::match(l, r) } -> std::same_as<bool>;
    };

    template <typename T>
    struct input_validate
    {
        static constexpr auto value
            = gate::traits::is_port_type_valid_v<T> && std::is_copy_constructible_v<T>;
    };

    template <typename T>
    struct input_validate<const T>: input_validate<T>
    {
    };

    template <typename T>
    struct input_validate<const T*>
    {
        // TODO: should T conform to be a valid port type?
        // Raw pointers can only be converted from something else. (like smart_ptrs and optional).
        // So, the only valid raw pointers would be coming from a valid port
        static constexpr auto value = gate::traits::is_port_type_valid_v<T>;
    };

    template <typename T>
    struct input_validate<const T&>
    {
        static constexpr auto value = gate::traits::is_port_type_valid_v<T>;
    };

    template <typename T>
    struct input_validate<const T&&>
    {
        static constexpr auto value = false;
    };

    template <typename T>
    struct input_validate<T*>
    {
        static constexpr auto value = false;
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
    concept input_validate_v = //
        input_validate<T>::value;

    template <typename T>
    concept req_output_validate_v =                                 //
        std::is_same_v<T, std::decay_t<T>>                          //
        && gate::traits::is_port_type_valid_v<traits::portify_t<T>> //
        && is_equally_comparable<T>;

    template <typename T>
    concept opt_output_validate_v                //
        = std::is_same_v<T, std::decay_t<T>>     //
        && gate::traits::is_port_type_valid_v<T> //
        && is_equally_comparable<T>;
}
