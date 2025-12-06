#pragma once

#include "../IRunner.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace nil::gate::runners
{
    class SoftBlocking final: public IRunner
    {
    public:
        SoftBlocking()
            : th([this]() { loop(); })
        {
        }

        ~SoftBlocking() noexcept override
        {
            {
                auto _ = std::unique_lock(mutex);
                stopped = true;
                cv.notify_one();
            }
            {
                auto _ = std::unique_lock(done_mutex);
                done_cv.notify_one();
            }
            th.join();
        }

        SoftBlocking(SoftBlocking&&) = delete;
        SoftBlocking(const SoftBlocking&) = delete;
        SoftBlocking& operator=(SoftBlocking&&) = delete;
        SoftBlocking& operator=(const SoftBlocking&) = delete;

        void run(std::function<std::span<INode* const>()> apply_changes) override
        {
            bool should_wait = true;
            {
                auto lock = std::unique_lock(mutex);
                should_wait = !is_running;
                tasks.emplace_back(std::move(apply_changes));
                cv.notify_one();
            }
            if (should_wait)
            {
                auto done_lock = std::unique_lock(done_mutex);
                done_cv.wait(done_lock);
            }
        }

    private:
        bool stopped = false;
        bool is_running = false;
        std::thread th;
        std::mutex mutex;
        std::condition_variable cv;

        std::mutex done_mutex;
        std::condition_variable done_cv;

        std::vector<std::function<std::span<INode* const>()>> tasks;

        void loop()
        {
            while (!stopped)
            {
                auto tasks_to_execute = [this]() -> decltype(tasks)
                {
                    auto lock = std::unique_lock(mutex);
                    if (tasks.empty())
                    {
                        is_running = false;
                        {
                            auto done_lock = std::unique_lock(done_mutex);
                            done_cv.notify_one();
                        }
                        cv.wait(lock);
                        if (stopped)
                        {
                            return {};
                        }
                    }
                    is_running = true;
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
