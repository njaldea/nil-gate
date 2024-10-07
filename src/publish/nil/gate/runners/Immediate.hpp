#pragma once

#include "../IRunner.hpp"

namespace nil::gate::runners
{
    class Immediate final: public IRunner
    {
    public:
        void run(
            Core* core,
            std::unique_ptr<ICallable<void()>> apply_changes,
            std::span<const std::unique_ptr<INode>> nodes
        ) override
        {
            if (apply_changes)
            {
                apply_changes->call();
            }
            for (const auto& node : nodes)
            {
                if (nullptr != node)
                {
                    node->run(core);
                }
            }
        }
    };
}
