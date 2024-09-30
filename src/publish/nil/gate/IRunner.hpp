#pragma once

#include "ICallable.hpp"
#include "INode.hpp"

#include <memory>
#include <span>

namespace nil::gate
{
    struct IRunner
    {
        IRunner() = default;
        virtual ~IRunner() noexcept = default;

        IRunner(IRunner&&) noexcept = default;
        IRunner& operator=(IRunner&&) noexcept = default;

        IRunner(const IRunner&) = default;
        IRunner& operator=(const IRunner&) = default;

        virtual void run(
            std::unique_ptr<ICallable<void()>> apply_changes,
            std::span<const std::unique_ptr<INode>> nodes
        ) = 0;
    };
}
