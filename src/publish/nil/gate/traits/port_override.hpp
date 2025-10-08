#pragma once

#include <optional>

namespace nil::gate::traits::port
{
    template <typename T>
    bool has_value(const T& /* value */)
    {
        return true;
    }

    template <typename T>
    bool is_eq(const T& current_value, const T& new_value)
    {
        if constexpr (requires() { current_value == new_value; })
        {
            return current_value == new_value;
        }
        else
        {
            return true;
        }
    }

    template <typename T>
    void unset(std::optional<T>& value)
    {
        value = {};
    }

    template <typename T>
    bool is_compatible(const T& /* current_port */, const T& /* incoming_port */)
    {
        return true;
    }
}
