#pragma once

// This header is only available for use if the user has actual dependency on boost asio

#include "../../IRunner.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <set>
#include <thread>

namespace nil::gate::runners::boost_asio
{
    class Parallel final: public IRunner
    {
        static std::thread create_thread(boost::asio::io_context& c)
        {
            return std::thread(
                [&c]()
                {
                    auto _ = boost::asio::make_work_guard(c);
                    c.run();
                }
            );
        }

    public:
        explicit Parallel(std::size_t count)
            : main_th(create_thread(main_context))
        {
            for (auto i = 0u; i < count; ++i)
            {
                exec_th.emplace_back(create_thread(exec_context));
            }
        }

        ~Parallel() noexcept override
        {
            main_context.stop();
            exec_context.stop();

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

        void run(Core* core, std::function<void()> apply_changes, std::span<INode* const> nodes)
            override
        {
            boost::asio::post(
                main_context,
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
                boost::asio::post(
                    exec_context,
                    [this, core, node]()
                    {
                        node->exec(core);
                        boost::asio::post(
                            main_context,
                            [this, core, node]() { this->mark_done(core, node); }
                        );
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

            // This is early frame stop. Is this desirable?
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
        std::set<nil::gate::INode*> running_list;
        std::vector<nil::gate::INode*> waiting_list;

        std::span<INode* const> deferred_nodes;
        std::vector<std::function<void()>> all_diffs;

        boost::asio::io_context main_context;
        boost::asio::io_context exec_context;

        std::thread main_th;
        std::vector<std::thread> exec_th;
    };
}
