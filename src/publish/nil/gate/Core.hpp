#pragma once

#include "Graph.hpp"
#include "IRunner.hpp"

#include "detail/traits/node.hpp"

#include <nil/xalt/fn_call.hpp>

namespace nil::gate
{
    class Core final
    {
    public:
        explicit Core(IRunner* init_runner = nullptr)
            : graph(this)
            , runner(init_runner)
        {
        }

        ~Core() noexcept = default;
        Core(Core&&) noexcept = delete;
        Core& operator=(Core&&) noexcept = delete;

        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        template <typename Callable>
        void post(Callable&& callable)
        {
            changes.push_back([callable = std::forward<Callable>(callable)](Graph& g) mutable
                              { nil::xalt::fn_call(callable, g); });
        }

        template <typename Callable>
        void apply(Callable&& callable)
        {
            post(std::forward<Callable>(callable));
            commit();
        }

        /**
         * Commit all of the changes to the graph.
         * Changes are scheduled by mutating the ports through their set_value method.
         */
        void commit()
        {
            if (runner == nullptr)
            {
                return;
            }
            runner->run(
                [this, fns = std::move(changes)]()
                {
                    for (const auto& fn : fns)
                    {
                        if (fn)
                        {
                            fn(graph);
                        }
                    }
                    return graph.sort();
                }
            );
        }

        /**
         * Use a custom runner. By default uses `nil::gate:runner::Immediate`.
         */
        void set_runner(IRunner* new_runner)
        {
            runner = new_runner;
        }

        IRunner* get_runner() const
        {
            return runner;
        }

    private:
        Graph graph;
        IRunner* runner = nullptr;
        std::vector<std::function<void(Graph&)>> changes;
    };

    template <typename TO, typename FROM>
    auto Graph::link(ports::ReadOnly<FROM>* from, ports::External<TO>* to)
        -> nil::gate::Node<nil::xalt::tlist<FROM>, nil::xalt::tlist<>>*
    {
        need_to_sort = true;
        static_assert(concepts::is_compatible<TO, FROM>, "Not Compatible");
        return this->node([mto = to->to_direct()](const FROM& v) { mto->set_value(v); }, {from});
    }
}
