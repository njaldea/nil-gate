#pragma once

#include "../IRunner.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

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
            stopped = true;
            {
                auto _ = std::unique_lock(mutex);
                cv.notify_one();
            }
            th.join();
        }

        NonBlocking(NonBlocking&&) = delete;
        NonBlocking(const NonBlocking&) = delete;
        NonBlocking& operator=(NonBlocking&&) = delete;
        NonBlocking& operator=(const NonBlocking&) = delete;

        void run(
            Core* core,
            std::unique_ptr<ICallable<void()>> apply_changes,
            std::span<const std::unique_ptr<INode>> nodes
        ) override
        {
            auto _ = std::unique_lock(mutex);
            queue.emplace(core, std::move(apply_changes), nodes);
            cv.notify_one();
        }

    private:
        std::atomic<bool> stopped = false;
        std::thread th;
        std::mutex mutex;
        std::condition_variable cv;

        struct Task // NOLINT
        {
            Core* core;
            std::unique_ptr<ICallable<void()>> apply_changes;
            std::span<const std::unique_ptr<INode>> nodes;
        };

        std::queue<Task> queue;

        void loop()
        {
            Task task;
            while (!stopped)
            {
                {
                    auto _ = std::unique_lock(mutex);
                    if (queue.empty())
                    {
                        cv.wait(_);
                    }
                    if (!queue.empty())
                    {
                        task = std::move(queue.front());
                        queue.pop();
                    }
                }
                if (task.apply_changes)
                {
                    task.apply_changes->call();
                }
                for (const auto& node : task.nodes)
                {
                    if (nullptr != node)
                    {
                        node->run(task.core);
                    }
                }
                task = Task();
            }
        }
    };
}
