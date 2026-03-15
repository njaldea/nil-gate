#pragma once

#include "../IRunner.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace nil::gate::runners
{
    template <typename TaskManager>
    class AsyncT: public IRunner
    {
    public:
        explicit AsyncT(std::size_t count)
            : main_tasks(1)
            , exec_tasks(count)
        {
        }

        ~AsyncT() noexcept override
        {
            main_tasks.stop();
            exec_tasks.stop();

            main_tasks.join();
            exec_tasks.join();
        }

        AsyncT(AsyncT&&) = delete;
        AsyncT(const AsyncT&) = delete;
        AsyncT& operator=(AsyncT&&) = delete;
        AsyncT& operator=(const AsyncT&) = delete;

        void run(std::function<std::span<INode* const>()> apply_changes) override
        {
            main_tasks.push(
                [this, apply_changes = std::move(apply_changes)]() mutable
                {
                    if (apply_changes)
                    {
                        all_diffs.emplace_back(std::move(apply_changes));
                    }

                    if (!running_list.empty())
                    {
                        return;
                    }

                    std::span<INode* const> nodes;
                    for (const auto& dd : std::exchange(all_diffs, {}))
                    {
                        if (dd)
                        {
                            nodes = dd();
                        }
                    }

                    auto max_score = 0U;
                    waiting_list.clear();
                    for (const auto& node : nodes)
                    {
                        if (node == nullptr)
                        {
                            continue;
                        }
                        max_score = std::max(max_score, node->score());
                    }
                    waiting_list.resize(max_score + 1);

                    current_score = 0U;
                    for (const auto& node : nodes)
                    {
                        if (node == nullptr)
                        {
                            continue;
                        }
                        waiting_list[node->score()].push_back(node);
                    }

                    run_score();
                }
            );
        }

    private:
        std::unordered_set<INode*> running_list;
        std::vector<std::vector<INode*>> waiting_list;

        std::vector<std::function<std::span<INode* const>()>> all_diffs;

        std::uint32_t current_score = 0U;
        TaskManager main_tasks;
        TaskManager exec_tasks;

        void run_node(INode* node)
        {
            if (node == nullptr || !node->is_pending() || !node->is_ready())
            {
                return;
            }

            running_list.emplace(node);
            exec_tasks.push(
                [this, node]()
                {
                    if (node->is_input_changed())
                    {
                        node->exec();
                    }
                    main_tasks.push([this, node]() { mark_done(node); });
                }
            );
        }

        void mark_done(INode* node)
        {
            node->done();
            running_list.erase(node);

            if (!all_diffs.empty())
            {
                if (running_list.empty())
                {
                    run({});
                }
            }
            else
            {
                if (running_list.empty())
                {
                    run_score();
                }
            }
        }

        void run_score()
        {
            if (current_score == waiting_list.size())
            {
                return;
            }

            if (!running_list.empty())
            {
                return;
            }

            for (const auto& n : waiting_list[current_score])
            {
                run_node(n);
            }
            waiting_list[current_score].clear();
            ++current_score;

            if (running_list.empty())
            {
                run_score();
            }
        }
    };
}
