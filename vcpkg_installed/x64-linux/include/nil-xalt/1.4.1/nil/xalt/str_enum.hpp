#pragma once

#include "literal.hpp"
#include "str_name.hpp"
#include "tlist.hpp"
#include "typed.hpp"

#include <string_view>
#include <type_traits>

namespace nil::xalt
{
    namespace detail
    {
        template <auto value>
        constexpr auto is_valid_enum = !starts_with<nil::xalt::str_name<typify<value>>(), "(">();
    }

    template <typename T>
    struct str_enum_start;

    template <typename T>
        requires(detail::is_valid_enum<T(0)>)
    struct str_enum_start<T>
    {
        static constexpr T value = T(0);
    };

    template <typename T>
        requires(!detail::is_valid_enum<T(0)> && detail::is_valid_enum<T(1)>)
    struct str_enum_start<T>
    {
        static constexpr T value = T(1);
    };

    template <typename T>
    constexpr T str_enum_start_v = str_enum_start<T>::value;

    namespace detail
    {
        template <auto v>
        concept has_single_bit = (v != 0) && !(v & (v - 1));

        template <typename T, auto v>
        concept is_masking //
            = (decltype(v)(str_enum_start_v<T>) > decltype(v)(0))
            && has_single_bit<decltype(v)(str_enum_start_v<T>)> // first should have only 1 bit
            && has_single_bit<decltype(v)(v)>;                  // current should have only 1 bit

        template <typename T, T v>
        constexpr auto next()
        {
            using type = decltype(v);
            constexpr auto raw_value = std::underlying_type_t<type>(v);
            constexpr auto is_masking_enum = is_masking<type, raw_value>;
            if constexpr (!is_masking_enum && is_valid_enum<type(raw_value + 1)>)
            {
                return type(raw_value + 1);
            }
            else if constexpr (is_masking_enum && is_valid_enum<type(raw_value << 1)>)
            {
                return type(raw_value << 1);
            }
            else
            {
                return str_enum_start_v<type>;
            }
        }

        template <typename T, typename L, T I>
        struct values;

        template <typename T, T... L, T I>
            requires(str_enum_start_v<T> != I)
        struct values<T, tlist<typify<L>...>, I> final
        {
            using type = typename values<T, tlist<typify<L>..., typify<T(I)>>, next<T, I>()>::type;
        };

        template <typename T, typename U, T I>
            requires(str_enum_start_v<T> == I)
        struct values<T, U, I> final
        {
            using type = U;
        };
    }

    template <typename T>
    struct str_enum_values final
    {
        static_assert(detail::is_valid_enum<str_enum_start_v<T>>);
        using type = typename detail::values< //
            T,
            tlist<typify<str_enum_start_v<T>>>,
            detail::next<T, str_enum_start_v<T>>()>::type;
    };

    template <typename T>
    using str_enum_values_t = typename str_enum_values<T>::type;

    template <typename T>
    std::string_view str_enum(T enum_value)
    {
        static constexpr auto each = []<T... v>(T value, tlist<typify<v>...>)
        {
            const char* name = nullptr;
            (void)(                                                                  //
                (nullptr != (name = (value == v ? str_name_v<typify<v>> : nullptr))) //
                || ...                                                               //
                || (name = "-")
            );
            return name;
        };
        return each(enum_value, str_enum_values_t<T>());
    }

    template <typename T>
    std::string_view str_short_enum(T enum_value)
    {
        static constexpr auto each = []<T... v>(T value, tlist<typify<v>...>)
        {
            const char* name = nullptr;
            (void)(                                                                        //
                (nullptr != (name = (value == v ? str_short_name_v<typify<v>> : nullptr))) //
                || ...                                                                     //
                || (name = "-")
            );
            return name;
        };
        return each(enum_value, str_enum_values_t<T>());
    }
}
