#pragma once

#include "INode.hpp"

#include <functional>
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
            std::function<void()> apply_changes,
            std::span<INode* const> nodes
        ) = 0;
    };
}
