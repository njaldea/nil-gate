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
}

namespace nil::gate::traits
{
    template <typename T>
    struct Port
    {
        static bool has_value(const T& value);
        static bool is_eq(const T& current_value, const T& new_value);
        static void unset(std::optional<T>& value);
    };

    template <typename T>
    bool Port<T>::has_value(const T& value)
    {
        return port::has_value(value);
    }

    template <typename T>
    bool Port<T>::is_eq(const T& current_value, const T& new_value)
    {
        return port::is_eq(current_value, new_value);
    }

    template <typename T>
    void Port<T>::unset(std::optional<T>& value)
    {
        port::unset(value);
    }
}
