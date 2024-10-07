#pragma once

#include "ICallable.hpp"
#include "INode.hpp"

#include <memory>
#include <span>

namespace nil::gate
{
    class Core;

    struct IRunner
    {
        IRunner() = default;
        virtual ~IRunner() noexcept = default;

        IRunner(IRunner&&) noexcept = default;
        IRunner& operator=(IRunner&&) noexcept = default;

        IRunner(const IRunner&) = default;
        IRunner& operator=(const IRunner&) = default;

        /**
         * @param apply_changes - a callable that is intended to apply the changes on the edges.
         *                      - the changes are mainly from Edge::set_value.
         * @param nodes - these nodes are alive as long as the Core object is alive.
         */
        virtual void run(
            Core* core,
            std::unique_ptr<ICallable<void()>> apply_changes,
            std::span<const std::unique_ptr<INode>> nodes
        ) = 0;
    };
}
