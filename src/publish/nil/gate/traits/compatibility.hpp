#pragma once

#include <type_traits>

namespace nil::gate::traits
{
    template <typename TO, typename FROM>
    struct compatibility
    {
        static const TO& convert(const FROM& u) = delete;
    };

    template <typename TYPE>
    struct compatibility<TYPE, TYPE>
    {
        static const TYPE& convert(const TYPE& u)
        {
            return u;
        }
    };
}

namespace nil::gate::concepts
{
    template <typename TO, typename FROM>
    concept compatibility_requires_cache = !std::is_reference_v<
        decltype(traits::compatibility<TO, FROM>::convert(std::declval<FROM>()))>;
}
