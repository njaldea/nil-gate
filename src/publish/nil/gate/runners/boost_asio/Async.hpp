#pragma once

// This header is only available for use if the user has actual dependency on boost asio
#include "../Async.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <thread>

namespace nil::gate::runners::boost_asio
{
    class TaskManager
    {
    public:
        explicit TaskManager(std::size_t thread_count)
        {
            for (std::size_t i = 0; i < thread_count; ++i)
            {
                threads.emplace_back(
                    [this]()
                    {
                        auto _ = boost::asio::make_work_guard(context);
                        context.run();
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
            context.stop();
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
            boost::asio::post(context, std::move(cb));
        }

    private:
        boost::asio::io_context context;
        std::vector<std::thread> threads;
    };

    using Async = AsyncT<TaskManager>;
}
