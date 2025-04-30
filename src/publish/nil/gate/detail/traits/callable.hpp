#pragma once

#include <nil/xalt/checks.hpp>
#include <nil/xalt/fn_sign.hpp>
#include <nil/xalt/tlist.hpp>

#include <tuple>

namespace nil::gate::detail::traits
{
    template <typename T>
    struct callable
    {
        using inputs = typename xalt::fn_sign<T>::arg_types;
        using outputs = xalt::tlist_types<typename xalt::fn_sign<T>::return_type>;
    };

    template <typename T>
        requires(xalt::is_of_template_v<typename xalt::fn_sign<T>::return_type, std::tuple>)
    struct callable<T>
    {
        using inputs = typename xalt::fn_sign<T>::arg_types;
        using outputs = typename xalt::to_tlist_types<typename xalt::fn_sign<T>::return_type>::type;
    };

    template <typename T>
        requires(std::is_same_v<typename xalt::fn_sign<T>::return_type, void>)
    struct callable<T>
    {
        using inputs = typename xalt::fn_sign<T>::arg_types;
        using outputs = xalt::tlist_types<>;
    };
}
