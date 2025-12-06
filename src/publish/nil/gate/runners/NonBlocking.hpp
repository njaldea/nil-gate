#pragma once

#include "../IRunner.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace nil::gate::runners
{
    class NonBlocking final: public IRunner
    {
    public:
        NonBlocking()
            : th([this]() { loop(); })
        {
        }

        ~NonBlocking() noexcept override
        {
            {
                auto _ = std::unique_lock(mutex);
                stopped = true;
                cv.notify_one();
            }
            th.join();
        }

        NonBlocking(NonBlocking&&) = delete;
        NonBlocking(const NonBlocking&) = delete;
        NonBlocking& operator=(NonBlocking&&) = delete;
        NonBlocking& operator=(const NonBlocking&) = delete;

        void run(std::function<std::span<INode* const>()> apply_changes) override
        {
            auto _ = std::unique_lock(mutex);
            tasks.emplace_back(std::move(apply_changes));
            cv.notify_one();
        }

    private:
        bool stopped = false;
        std::thread th;
        std::mutex mutex;
        std::condition_variable cv;

        std::vector<std::function<std::span<INode* const>()>> tasks;

        void loop()
        {
            while (!stopped)
            {
                auto tasks_to_execute = [this]() -> decltype(tasks)
                {
                    auto _ = std::unique_lock(mutex);
                    if (tasks.empty())
                    {
                        cv.wait(_);
                        if (stopped)
                        {
                            return {};
                        }
                    }
                    return std::exchange(tasks, {});
                }();
                if (!tasks_to_execute.empty())
                {
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
        }
    };
}
