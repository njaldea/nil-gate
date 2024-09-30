#pragma once

#include "ICallable.hpp"
#include "INode.hpp"

#include <vector>

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

        /**
         * @param invokable these are changes needed to be applied to the graph
         */
        virtual void flush(std::unique_ptr<ICallable<void()>> invokable) = 0;
        /**
         * @param nodes lifetime of these nodes is owned by the parent Core.
         */
        virtual void run(const std::vector<std::unique_ptr<INode>>& nodes) = 0;
    };
}
