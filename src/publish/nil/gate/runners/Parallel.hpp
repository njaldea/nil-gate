#pragma once

#include "../IRunner.hpp"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <thread>

namespace nil::gate::runners
{
    class Parallel final: public IRunner
    {
    public:
        explicit Parallel(std::size_t count)
            : main_tasks(1)
            , exec_tasks(count)
        {
        }

        ~Parallel() noexcept override = default;

        Parallel(Parallel&&) = delete;
        Parallel(const Parallel&) = delete;
        Parallel& operator=(Parallel&&) = delete;
        Parallel& operator=(const Parallel&) = delete;

        void run(Core* core, std::function<void()> apply_changes, std::span<INode* const> nodes)
            override
        {
            main_tasks.push(
                [this, core, nodes, apply_changes = std::move(apply_changes)]() mutable
                {
                    all_diffs.emplace_back(std::move(apply_changes));
                    if (running_list.empty())
                    {
                        for (const auto& dd : std::exchange(all_diffs, {}))
                        {
                            if (dd)
                            {
                                dd();
                            }
                        }
                    }

                    if (!running_list.empty())
                    {
                        deferred_nodes = nodes;
                        return;
                    }

                    for (const auto& node : nodes)
                    {
                        if (nullptr != node && node->is_pending())
                        {
                            if (node->is_ready())
                            {
                                run_node(core, node);
                            }
                            else
                            {
                                waiting_list.push_back(node);
                            }
                        }
                    }
                }
            );
        }

        void run_node(Core* core, INode* node)
        {
            if (node->is_input_changed())
            {
                running_list.emplace(node);
                exec_tasks.push(
                    [this, core, node]()
                    {
                        node->exec(core);
                        main_tasks.push([this, core, node]() { mark_done(core, node); });
                    }
                );
            }
            else
            {
                mark_done(core, node);
            }
        }

        void mark_done(Core* core, INode* node)
        {
            node->done();
            running_list.erase(node);

            if (!deferred_nodes.empty())
            {
                if (running_list.empty())
                {
                    waiting_list.clear();
                    run(core, {}, deferred_nodes);
                    deferred_nodes = {};
                }
            }
            else
            {
                std::erase_if(
                    waiting_list,
                    [this, core](auto* n)
                    {
                        if (n->is_ready())
                        {
                            run_node(core, n);
                            return true;
                        }
                        return false;
                    }
                );
            }
        }

    private:
        class TaskManager
        {
        private:
            class Queue
            {
            public:
                void push(std::function<void()> task)
                {
                    {
                        std::unique_lock lock(mutex_);
                        tasks.push(std::move(task));
                    }
                    cv.notify_one();
                }

                std::optional<std::function<void()>> pop()
                {
                    std::unique_lock lock(mutex_);
                    cv.wait(lock, [&] { return stop_flag || !tasks.empty(); });
                    if (stop_flag && tasks.empty())
                    {
                        return std::nullopt;
                    }

                    auto task = std::move(tasks.front());
                    tasks.pop();
                    return task;
                }

                void stop()
                {
                    {
                        std::unique_lock lock(mutex_);
                        stop_flag = true;
                    }
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
                                (*task)();
                            }
                        }
                    );
                }
            }

            TaskManager(TaskManager&&) = delete;
            TaskManager(const TaskManager&) = delete;
            TaskManager& operator=(TaskManager&&) = delete;
            TaskManager& operator=(const TaskManager&) = delete;

            ~TaskManager()
            {
                queue.stop();
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

        std::set<INode*> running_list;
        std::vector<INode*> waiting_list;

        std::span<INode* const> deferred_nodes;
        std::vector<std::function<void()>> all_diffs;

        TaskManager main_tasks;
        TaskManager exec_tasks;
    };
}
