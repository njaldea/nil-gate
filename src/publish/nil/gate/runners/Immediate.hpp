#pragma once

#include "../IRunner.hpp"

namespace nil::gate::runners
{
    class Immediate final: public IRunner
    {
    public:
        void flush(std::unique_ptr<ICallable<void()>> invokable) override
        {
            if (invokable)
            {
                invokable->call();
            }
        }

        void run(const std::vector<std::unique_ptr<INode>>& nodes) override
        {
            for (const auto& node : nodes)
            {
                node->run();
            }
        }
    };
}
