#pragma once

namespace nil::xalt
{
    template <typename T, template <typename...> typename U>
    struct is_of_template final
    {
        static constexpr bool value = false;
    };

    template <typename... P, template <typename...> typename U>
    struct is_of_template<U<P...>, U> final
    {
        static constexpr bool value = true;
    };

    template <typename T, template <typename...> typename U>
    concept is_of_template_v = is_of_template<T, U>::value;
}
