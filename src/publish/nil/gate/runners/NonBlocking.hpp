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

        void run(std::function<void()> apply_changes, std::span<INode* const> nodes) override
        {
            auto _ = std::unique_lock(mutex);
            tasks.emplace_back(std::move(apply_changes), nodes);
            cv.notify_one();
        }

    private:
        bool stopped = false;
        std::thread th;
        std::mutex mutex;
        std::condition_variable cv;

        struct Task // NOLINT
        {
            std::function<void()> apply_changes;
            std::span<INode* const> nodes;
        };

        std::vector<Task> tasks;

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
                    for (const auto& task : tasks_to_execute)
                    {
                        if (task.apply_changes)
                        {
                            task.apply_changes();
                        }
                    }
                    const auto& t = tasks_to_execute.back();
                    for (const auto& node : t.nodes)
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
