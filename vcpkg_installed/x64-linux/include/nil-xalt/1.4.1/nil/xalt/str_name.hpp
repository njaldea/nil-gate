#pragma once

#if !defined(__clang__) && !defined(__GNUC__) && !defined(_MSC_VER)
#include "errors.hpp"
#endif

#include "literal.hpp"
#include "typed.hpp"

namespace nil::xalt::detail
{
    template <literal U>
    static consteval auto pretty_function()
    {
        constexpr auto pretty_print = literal_sv<U>;

        constexpr auto pos1 = pretty_print.find_first_of('=') + 2;
        constexpr auto pos2 = pretty_print.find_last_of(']');

        return literal<pos2 - pos1 + 1>(pretty_print.data(), pos1);
    }

    template <literal U>
    static consteval auto funcsig()
    {
        constexpr auto pretty_print = literal_sv<U>;

        constexpr auto p = pretty_print.find_first_of('<');
        constexpr auto f_e = pretty_print.find("<enum ", p);
        constexpr auto f_c = pretty_print.find("<class ", p);
        constexpr auto f_s = pretty_print.find("<struct ", p);

        constexpr auto pos1 = p + (f_e == p ? 6 : (f_c == p ? 7 : (f_s == p ? 8 : 1)));
        constexpr auto pos2 = pretty_print.find_last_of('>');

        return literal<pos2 - pos1 + 1>(pretty_print.data(), pos1);
    }

    template <typename T>
    consteval auto str_lit()
    {
#if defined(__clang__) || defined(__GNUC__)
        return detail::pretty_function<__PRETTY_FUNCTION__>();
#elif defined(_MSC_VER)
        return detail::funcsig<__FUNCSIG__>();
#else
        undefined<T>();
        return "not supported";
#endif
    }

    template <auto T>
    consteval auto str_lit()
    {
#if defined(__clang__) || defined(__GNUC__)
        return detail::pretty_function<__PRETTY_FUNCTION__>();
#elif defined(_MSC_VER)
        return detail::funcsig<__FUNCSIG__>();
#else
        undefined<T>();
        return "not supported";
#endif
    }

    template <typename T>
    struct str_name_dispatch
    {
        static consteval auto name()
        {
            return str_lit<T>();
        }
    };

    template <auto T>
    struct str_name_dispatch<typify<T>>
    {
        static consteval auto name()
        {
            return str_lit<T>();
        }
    };

    template <literal T>
    consteval auto str_name_index() -> std::size_t
    {
        constexpr auto name_sv = literal_sv<T>;
        constexpr auto is_templated = name_sv.ends_with('>');
        auto angle_check = is_templated;
        auto track = 0;
        for (auto i = 0UL; i < name_sv.size(); ++i)
        {
            if (angle_check)
            {
                if (name_sv[name_sv.size() - i - 1] == '>')
                {
                    track += 1;
                }
                if (name_sv[name_sv.size() - i - 1] == '<')
                {
                    track -= 1;
                }
                if (track == 0)
                {
                    angle_check = false;
                }
            }
            else
            {
                if (name_sv[name_sv.size() - i - 1] == ':')
                {
                    return name_sv.size() - i;
                }
            }
        }
        return 0UL;
    }
}

namespace nil::xalt
{
    template <typename T>
    consteval auto str_name()
    {
        return detail::str_name_dispatch<T>::name();
    }

    template <typename T>
    consteval auto str_short_name()
    {
        constexpr auto name = str_name<T>();
        static_assert(!starts_with<name, "(">());
        return substr<name, detail::str_name_index<name>()>();
    }

    template <typename T>
    consteval auto str_scope_name()
    {
        constexpr auto name = str_name<T>();
        static_assert(!starts_with<name, "(">());
        constexpr auto index = detail::str_name_index<name>();
        return substr<name, 0, index - (index > 2UL ? 2UL : 0UL)>();
    }

    template <typename T>
    inline constexpr const auto& str_name_v = literal_v<str_name<T>()>;

    template <typename T>
    inline constexpr const auto& str_name_sv = literal_sv<str_name<T>()>;

    template <typename T>
    inline constexpr const auto& str_short_name_v = literal_v<str_short_name<T>()>;

    template <typename T>
    inline constexpr const auto& str_short_name_sv = literal_sv<str_short_name<T>()>;

    template <typename T>
    inline constexpr const auto& str_scope_name_v = literal_v<str_scope_name<T>()>;

    template <typename T>
    inline constexpr const auto& str_scope_name_sv = literal_sv<str_scope_name<T>()>;
}
