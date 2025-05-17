#pragma once

#include "../traits/portify.hpp"

#include <memory>
#include <optional>

namespace nil::gate::traits
{
    template <typename T>
    struct portify<std::unique_ptr<T>>
    {
        using type = std::unique_ptr<const T>;
    };

    template <typename T>
    struct portify<std::shared_ptr<T>>
    {
        using type = std::shared_ptr<const T>;
    };

    template <typename T>
    struct portify<std::optional<T>>
    {
        using type = std::optional<const T>;
    };

    template <typename T>
    struct portify<std::reference_wrapper<T>>
    {
        using type = std::reference_wrapper<const T>;
    };
}
