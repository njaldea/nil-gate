#pragma once

namespace nil::gate::traits
{
    template <typename T>
    struct portify
    {
        using type = T;
    };

    template <typename T>
    using portify_t = typename portify<T>::type;
}
