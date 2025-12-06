#pragma once

#include "Core.hpp"
#include "nil/gate/IRunner.hpp"
#include "nil/gate/ports/Port.hpp"

namespace nil::gate
{
    /**
     * Uniform node registration helper.
     *
     * Provides a single entry point that hides the four Core::node signature variants:
     *  - node with inputs & outputs
     *  - node with inputs & no outputs
     *  - node with outputs & no inputs
     *  - node with neither inputs nor outputs
     *
     * Always returns a std::tuple of fix + opt output ports when the callable declares outputs;
     * otherwise returns an empty std::tuple<>.
     *
     * Requirements (validated by underlying traits):
     *  - Optional first parameter: Core&
     *  - Optional second parameter: opt_output<...>
     *  - Remaining parameters: inputs (zero or more)
     *  - Return: void, T, or std::tuple<T...>
     *
     * NOTE: For nodes with zero inputs you may pass an empty tuple, or call the raw Core::node
     * overload directly.
     */
    template <typename T>
    auto add_node(
        Graph& graph,
        T&& instance,
        typename detail::traits::node<T>::inputs::ports inputs
    )
    {
        constexpr auto has_output = detail::traits::node<T>::outputs::size > 0;
        constexpr auto has_input = detail::traits::node<T>::inputs::size > 0;

        if constexpr (has_output && has_input)
        {
            return graph.node(std::forward<T>(instance), inputs);
        }
        if constexpr (has_output && !has_input)
        {
            return graph.node(std::forward<T>(instance));
        }
        if constexpr (!has_output && has_input)
        {
            graph.node(std::forward<T>(instance), inputs);
            return std::tuple<>();
        }
        if constexpr (!has_output && !has_input)
        {
            graph.node(std::forward<T>(instance));
            return std::tuple<>();
        }
    }

    /**
     * Create an uninitialized mutable port of type T.
     * Port becomes ready after first set_value()/sync write.
     */
    template <typename T>
    auto add_port(Graph& graph)
    {
        return graph.port<T>();
    }

    /**
     * Create a mutable port initialized with a value (immediately ready).
     */
    template <typename T>
    auto add_port(Graph& graph, T&& value)
    {
        return graph.port(std::forward<T>(value));
    }

    /**
     * Link a ReadOnly<FROM> producing port to a Port<TO> consuming port.
     * Type adaptation (if any) is handled through traits::compatibility.
     */
    template <typename TO, typename FROM>
    auto link(Graph& graph, ports::ReadOnly<FROM>* from, Port<TO>* to)
    {
        return graph.link(from, to);
    }

    /**
     * Replace current runner with a new instance of T constructed in-place with Args.
     */
    inline void set_runner(Core& core, IRunner* runner)
    {
        core.set_runner(runner);
    }
}
