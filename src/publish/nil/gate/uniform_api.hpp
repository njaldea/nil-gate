#pragma once

#include "Core.hpp"
#include "nil/gate/IRunner.hpp"

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
     *  - Optional first parameter: const Core&
     *  - Optional second parameter: opt_output<...>
     *  - Remaining parameters: inputs (zero or more)
     *  - Return: void, T, or std::tuple<T...>
     *
     * NOTE: For nodes with zero inputs you may pass an empty tuple, or call the raw Core::node
     * overload directly.
     */
    template <typename T>
    auto add_node(Core& core, T&& instance, typename detail::traits::node<T>::inputs::ports inputs)
    {
        constexpr auto has_output = detail::traits::node<T>::outputs::size > 0;
        constexpr auto has_input = detail::traits::node<T>::inputs::size > 0;

        if constexpr (has_output && has_input)
        {
            return core.node(std::forward<T>(instance), inputs);
        }
        if constexpr (has_output && !has_input)
        {
            return core.node(std::forward<T>(instance));
        }
        if constexpr (!has_output && has_input)
        {
            core.node(std::forward<T>(instance), inputs);
            return std::tuple<>();
        }
        if constexpr (!has_output && !has_input)
        {
            core.node(std::forward<T>(instance));
            return std::tuple<>();
        }
    }

    /**
     * Create an uninitialized mutable port of type T.
     * Port becomes ready after first set_value()/sync write.
     */
    template <typename T>
    auto add_port(Core& core)
    {
        return core.port<T>();
    }

    /**
     * Create a mutable port initialized with a value (immediately ready).
     */
    template <typename T>
    auto add_port(Core& core, T&& value)
    {
        return core.port(std::forward<T>(value));
    }

    /**
     * Link a ReadOnly<FROM> producing port to a Mutable<TO> consuming port.
     * Type adaptation (if any) is handled through traits::compatibility.
     */
    template <typename TO, typename FROM>
    void link(Core& core, ports::ReadOnly<FROM>* from, ports::Mutable<TO>* to)
    {
        core.link(from, to);
    }

    /**
     * Begin a batch over the provided mutable ports (variadic form).
     * Writes via the returned Batch<T...> appear atomically at next commit.
     */
    template <typename... T>
    Batch<T...> batch(const Core& core, ports::Mutable<T>*... ports)
    {
        return core.batch(ports...);
    }

    /**
     * Begin a batch from a tuple of mutable ports.
     */
    template <typename... T>
    Batch<T...> batch(const Core& core, std::tuple<ports::Mutable<T>*...> ports)
    {
        return core.batch(std::move(ports));
    }

    /**
     * Replace current runner with a new instance of T constructed in-place with Args.
     */
    void set_runner(Core& core, IRunner* runner)
    {
        core.set_runner(runner);
    }

    /**
     * Replace current runner with a new instance of T constructed in-place with Args.
     */
    template <typename T>
    void set_runner(Core& core, T&& runner)
    {
        core.set_runner(std::forward<T>(runner));
    }

    /** Clear all ports and nodes (invalidates existing pointers). */
    inline void clear(Core& core)
    {
        core.clear();
    }
}
