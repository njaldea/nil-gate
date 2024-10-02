#pragma once

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
