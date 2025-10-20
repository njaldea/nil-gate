#pragma once

#include <optional>

namespace nil::gate::traits
{
    template <typename T>
    struct Port;
}

namespace nil::gate::traits::port
{
    template <typename T>
    bool has_value(const T& value)
    {
        if constexpr (requires() { Port<T>::has_value(value); })
        {
            return Port<T>::has_value(value);
        }
        else
        {
            return true;
        }
    }

    template <typename T>
    bool is_eq(const T& current_value, const T& new_value)
    {
        if constexpr (requires() { Port<T>::is_eq(current_value, new_value); })
        {
            return Port<T>::is_eq(current_value, new_value);
        }
        else
        {
            return current_value == new_value;
        }
    }

    template <typename T>
    void unset(std::optional<T>& value)
    {
        if constexpr (requires() { Port<T>::unset(value); })
        {
            Port<T>::unset(value);
        }
        else
        {
            value = {};
        }
    }
}
