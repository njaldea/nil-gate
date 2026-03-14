#pragma once

#include "AsyncT.hpp"

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace nil::gate::runners
{
    class TaskManager
    {
    private:
        class Queue
        {
        public:
            void push(std::function<void()> task)
            {
                std::unique_lock lock(mutex_);
                tasks.push(std::move(task));
                cv.notify_one();
            }

            std::function<void()> pop()
            {
                std::unique_lock lock(mutex_);
                cv.wait(lock, [&] { return stop_flag || !tasks.empty(); });
                if (stop_flag || tasks.empty())
                {
                    return {};
                }

                auto task = std::move(tasks.front());
                tasks.pop();
                return task;
            }

            void stop()
            {
                std::unique_lock lock(mutex_);
                stop_flag = true;
                cv.notify_all();
            }

        private:
            std::queue<std::function<void()>> tasks;
            std::mutex mutex_;
            std::condition_variable cv;
            bool stop_flag = false;
        };

    public:
        explicit TaskManager(std::size_t thread_count)
        {
            for (std::size_t i = 0; i < thread_count; ++i)
            {
                threads.emplace_back(
                    [this]()
                    {
                        while (true)
                        {
                            auto task = queue.pop();
                            if (!task)
                            {
                                break;
                            }
                            task();
                        }
                    }
                );
            }
        }

        TaskManager(TaskManager&&) = delete;
        TaskManager(const TaskManager&) = delete;
        TaskManager& operator=(TaskManager&&) = delete;
        TaskManager& operator=(const TaskManager&) = delete;

        ~TaskManager() = default;

        void stop()
        {
            queue.stop();
        }

        void join()
        {
            for (auto& t : threads)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }
        }

        template <typename T>
        void push(T cb)
        {
            queue.push(std::move(cb));
        }

    private:
        Queue queue;
        std::vector<std::thread> threads;
    };

    using Async = AsyncT<TaskManager>;
}
