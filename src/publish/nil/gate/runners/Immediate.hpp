#pragma once

#include "../IRunner.hpp"

namespace nil::gate::runners
{
    class Immediate final: public IRunner
    {
    public:
        void run(std::function<std::span<INode* const>()> apply_changes) override
        {
            if (!apply_changes)
            {
                return;
            }

            for (const auto& node : apply_changes())
            {
                if (nullptr != node)
                {
                    node->run();
                }
            }
        }
    };
}
