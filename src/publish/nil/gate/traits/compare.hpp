#pragma once

namespace nil::gate::traits
{
    template <typename T>
    struct compare
    {
        static bool match(const T& l, const T& r)
        {
            return l == r;
        }
    };
}
