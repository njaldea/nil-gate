#pragma once

#include <type_traits>

namespace nil::gate::traits
{
    template <typename T>
    struct is_edge_type_valid: std::true_type
    {
    };

    template <typename T>
    struct is_edge_type_valid<T*>: std::false_type
    {
    };

    template <typename T>
    constexpr auto is_edge_type_valid_v = is_edge_type_valid<T>::value;
};
