#pragma once

#include "../IRunner.hpp"

#include <mutex>
#include <utility>
#include <vector>

namespace nil::gate::runners
{
    class SoftBlocking final: public IRunner
    {
    public:
        SoftBlocking() = default;

        ~SoftBlocking() noexcept override = default;

        SoftBlocking(SoftBlocking&&) = delete;
        SoftBlocking(const SoftBlocking&) = delete;
        SoftBlocking& operator=(SoftBlocking&&) = delete;
        SoftBlocking& operator=(const SoftBlocking&) = delete;

        void run(std::function<std::span<INode* const>()> apply_changes) override
        {
            bool should_use_this_thread = true;
            {
                auto lock = std::unique_lock(mutex);
                tasks.emplace_back(std::move(apply_changes));
                should_use_this_thread = !is_running;
                is_running = true;
            }

            if (!should_use_this_thread)
            {
                return;
            }

            while (true)
            {
                auto tasks_to_execute = [this]() -> decltype(tasks)
                {
                    auto lock = std::unique_lock(mutex);
                    is_running = !tasks.empty();
                    return std::exchange(tasks, {});
                }();

                if (tasks_to_execute.empty())
                {
                    break;
                }

                std::span<INode* const> nodes;
                for (const auto& task : tasks_to_execute)
                {
                    if (task)
                    {
                        nodes = task();
                    }
                }

                for (const auto& node : nodes)
                {
                    if (nullptr != node)
                    {
                        node->run();
                    }
                }
            }
        }

    private:
        bool is_running = false;
        std::mutex mutex;

        std::vector<std::function<std::span<INode* const>()>> tasks;
    };
}
