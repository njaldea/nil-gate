#pragma once

#include <type_traits>

namespace nil::gate::traits
{
    template <typename T>
    struct is_port_type_valid: std::false_type
    {
    };

    template <typename T>
        requires(std::is_same_v<T, std::decay_t<T>>)
    struct is_port_type_valid<T>: std::true_type
    {
    };

    template <typename T>
    struct is_port_type_valid<T*>: std::false_type
    {
    };

    template <typename T>
    concept is_port_type_valid_v = is_port_type_valid<T>::value;
};
