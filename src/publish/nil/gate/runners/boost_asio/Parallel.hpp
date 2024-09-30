#pragma once

// This header is only available for use if the user has actual dependency on boost asio

#include "../../IRunner.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <memory>
#include <set>
#include <thread>
#include <vector>

namespace nil::gate::runners::boost_asio
{
    class Parallel final: public nil::gate::IRunner
    {
    public:
        explicit Parallel(std::size_t count)
            : main_work(boost::asio::make_work_guard(main_context))
            , exec_work(boost::asio::make_work_guard(exec_context))
            , main_th([this]() { main_context.run(); })
        {
            for (auto i = 0u; i < count; ++i)
            {
                exec_th.emplace_back([this]() { exec_context.run(); });
            }
        }

        ~Parallel() noexcept override
        {
            main_context.stop();
            exec_context.stop();

            main_work.reset();
            exec_work.reset();

            main_th.join();

            for (auto& t : exec_th)
            {
                t.join();
            }
        }

        Parallel(Parallel&&) = delete;
        Parallel(const Parallel&) = delete;
        Parallel& operator=(Parallel&&) = delete;
        Parallel& operator=(const Parallel&) = delete;

        void flush(std::unique_ptr<nil::gate::ICallable<void()>> invokable) override
        {
            boost::asio::post(
                main_context,
                [this, invokable = std::move(invokable)]() mutable
                {
                    all_diffs.emplace_back(std::move(invokable));
                    if (running_list.empty())
                    {
                        for (const auto& dd : std::exchange(all_diffs, {}))
                        {
                            if (dd)
                            {
                                dd->call();
                            }
                        }
                    }
                }
            );
        }

        void run(const std::vector<std::unique_ptr<nil::gate::INode>>& nodes) override
        {
            boost::asio::post(
                main_context,
                [this, &nodes]()
                {
                    if (!running_list.empty())
                    {
                        deferred_nodes = &nodes;
                        return;
                    }
                    for (const auto& node : nodes)
                    {
                        if (node->is_pending())
                        {
                            if (node->is_ready())
                            {
                                run_node(node.get());
                            }
                            else
                            {
                                waiting_list.emplace(node.get());
                            }
                        }
                    }
                }
            );
        }

        void run_node(nil::gate::INode* node)
        {
            running_list.emplace(node);
            boost::asio::post(
                exec_context,
                [this, node]()
                {
                    node->exec();
                    mark_done(node);
                }
            );
        }

        void mark_done(nil::gate::INode* node)
        {
            boost::asio::post(
                main_context,
                [this, node]()
                {
                    node->done();
                    running_list.erase(node);

                    // This is early frame stop. Is this desirable?
                    if (nullptr != deferred_nodes)
                    {
                        if (running_list.empty())
                        {
                            waiting_list.clear();
                            flush({});
                            run(*deferred_nodes);
                        }
                    }
                    else
                    {
                        for (auto* n : waiting_list)
                        {
                            if (n->is_ready())
                            {
                                waiting_list.erase(n);
                                run_node(n);
                            }
                        }
                    }
                }
            );
        }

    private:
        std::set<nil::gate::INode*> running_list;
        std::set<nil::gate::INode*> waiting_list;

        const std::vector<std::unique_ptr<nil::gate::INode>>* deferred_nodes = nullptr;
        std::vector<std::unique_ptr<nil::gate::ICallable<void()>>> all_diffs;

        boost::asio::io_context main_context;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> main_work;

        boost::asio::io_context exec_context;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> exec_work;

        std::thread main_th;
        std::vector<std::thread> exec_th;
    };
}
