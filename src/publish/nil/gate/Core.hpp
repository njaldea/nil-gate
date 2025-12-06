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
        explicit Core(IRunner* init_runner)
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
    auto Graph::link(ports::ReadOnly<FROM>* from, Port<TO>* to)
        -> nil::gate::Node<nil::xalt::tlist<FROM>, nil::xalt::tlist<>>*
    {
        need_to_sort = true;
        static_assert(concepts::is_compatible<TO, FROM>, "Not Compatible");
        return this->node([to](const FROM& v) { to->set_value(v); }, {from});
    }

    template <typename T>
    void Port<T>::set_value(T new_data)
    {
        this->core->post([this, new_data = std::move(new_data)]() mutable
                         { this->port->set_value(std::move(new_data)); });
    }

    template <typename T>
    void Port<T>::unset_value()
    {
        this->core->post([this]() mutable { this->port->unset_value(); });
    }
}
