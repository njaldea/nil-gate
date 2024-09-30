#pragma once

#include "../Core.hpp"

namespace nil::gate::api::uniform
{
    template <typename T>
    auto add_node(
        T instance,
        nil::gate::Core& core,
        typename nil::gate::detail::traits::node<T>::inputs::edges inputs
    )
    {
        constexpr auto has_output = nil::gate::detail::traits::node<T>::outputs::size > 0;
        constexpr auto has_input = nil::gate::detail::traits::node<T>::inputs::size > 0;

        if constexpr (has_output && has_input)
        {
            return core.node(std::move(instance), inputs);
        }
        if constexpr (has_output && !has_input)
        {
            return core.node(std::move(instance));
        }
        if constexpr (!has_output && has_input)
        {
            core.node(std::move(instance), inputs);
            return std::tuple<>();
        }
        if constexpr (!has_output && !has_input)
        {
            core.node(std::move(instance));
            return std::tuple<>();
        }
    }
}
