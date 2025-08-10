#pragma once

#include "../IRunner.hpp"

namespace nil::gate::runners
{
    class Immediate final: public IRunner
    {
    public:
        void run(std::function<void()> apply_changes, std::span<INode* const> nodes) override
        {
            if (apply_changes)
            {
                apply_changes();
            }
            for (const auto& node : nodes)
            {
                if (nullptr != node)
                {
                    node->run();
                }
            }
        }
    };
}
