#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace nil::xalt
{
    template <std::size_t N>
    struct literal final
    {
        consteval literal(const char* init_value, std::size_t offset)
        {
            std::copy_n(&init_value[offset], N - 1, &private_value[0]);
            private_value[N - 1] = '\0';
        }

        // NOLINTNEXTLINE(hicpp-explicit-conversions,*-avoid-c-arrays)
        consteval literal(const char (&init_value)[N])
        {
            std::copy_n(&init_value[0], N, &private_value[0]);
        }

        template <std::size_t M, std::size_t... Rest>
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        consteval literal(literal<M> init_value_l, literal<Rest>... init_value_r)
        {
            auto it = std::copy_n(&init_value_l.private_value[0], M - 1, &private_value[0]);
            (..., (it = std::copy_n(&init_value_r.private_value[0], Rest - 1, it)));
        }

        // NOLINTNEXTLINE(*-avoid-c-arrays)
        char private_value[N] = {};
    };

    namespace literals
    {
        template <literal l>
        consteval const auto& operator""_lit()
        {
            return l;
        }
    }

    template <literal T>
    inline constexpr const auto& literal_v = T.private_value;
    template <literal T>
    inline constexpr const auto literal_sv = std::string_view(T.private_value);

    template <literal... T>
    consteval auto concat() -> literal<(1 + ... + (sizeof(T) - 1))>
    {
        return literal<(1 + ... + (sizeof(T) - 1))>(T...);
    }

    template <literal N, std::size_t pos, std::size_t count = sizeof(N) - pos>
    consteval auto substr() -> literal<count + 1>
    {
        static_assert(pos + count <= sizeof(N));
        return literal<count + 1>(literal_v<N>, pos);
    }

    template <literal from, literal to_find>
    consteval auto find() -> std::size_t
    {
        constexpr auto i = literal_sv<from>.find(literal_sv<to_find>);
        return i == std::string_view::npos ? sizeof(from) : i;
    }

    template <literal from, literal to_find>
    consteval auto rfind() -> std::size_t
    {
        constexpr auto i = literal_sv<from>.rfind(literal_sv<to_find>);
        return i == std::string_view::npos ? sizeof(from) : i;
    }

    template <literal from, literal to_find>
    consteval auto starts_with() -> bool
    {
        return literal_sv<from>.starts_with(literal_sv<to_find>);
    }

    template <literal from, literal to_find>
    consteval auto ends_with() -> bool
    {
        return literal_sv<from>.ends_with(literal_sv<to_find>);
    }

    template <literal base, literal from, literal to, std::size_t count>
    consteval auto replace()
    {
        if constexpr (constexpr auto index1 = find<base, from>(); index1 != sizeof(base))
        {
            constexpr auto index2 = index1 + sizeof(from) - 1;
            constexpr auto remaining_size = sizeof(base) - index2;
            constexpr auto section1 = substr<base, 0, index1>();
            constexpr auto section2 = substr<base, index2, remaining_size>();

            if constexpr (count == 1) {
                return concat<section1, to, section2>();
            } else {
                return concat<section1, to, replace<section2, from, to, count - 1>()>();
            }
        }
        else
        {
            return base;
        }
    }

    template <literal base, literal from, literal to>
    consteval auto replace_one()
    {
        return replace<base, from, to, 1>();
    }


    template <literal base, literal from, literal to>
    consteval auto replace_all()
    {
        return replace<base, from, to, 0xFFFFFFFF>();
    }
}
