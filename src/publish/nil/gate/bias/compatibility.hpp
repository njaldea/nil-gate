#pragma once

#include "../traits/compatibility.hpp"

#include <functional>
#include <memory>
#include <optional>

namespace nil::gate::traits
{
    template <typename T>
    struct compatibility<T, std::reference_wrapper<const T>>
    {
        static const T& convert(const std::reference_wrapper<const T>& u)
        {
            return u;
        }
    };

    template <typename T>
    struct compatibility<std::reference_wrapper<const T>, T>
    {
        static std::reference_wrapper<const T> convert(const T& u)
        {
            return std::cref(u);
        }
    };

    // NO pointer to ref since pointers can't guarantee that there is always a value
    // This compatibility layer is to allow a port to be adapted to a
    // different type of port for input of a node

    template <typename T>
    struct compatibility<const T*, std::unique_ptr<const T>>
    {
        static const T* convert(const std::unique_ptr<const T>& u)
        {
            return u.get();
        }
    };

    template <typename T>
    struct compatibility<const T*, std::shared_ptr<const T>>
    {
        static const T* convert(const std::shared_ptr<const T>& u)
        {
            return u.get();
        }
    };

    template <typename T>
    struct compatibility<const T*, std::optional<const T>>
    {
        static const T* convert(const std::optional<const T>& u)
        {
            return u.has_value() ? &u.value() : nullptr;
        }
    };
}
