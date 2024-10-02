#pragma once

#include "Batch.hpp"
#include "Diffs.hpp"
#include "errors.hpp"
#include "runners/Immediate.hpp"

#include "edges/Mutable.hpp"
#include "traits/edgify.hpp"

#include "detail/Node.hpp"
#include "detail/traits/node.hpp"

#include <vector>

namespace nil::gate::concepts
{
    template <typename T>
    concept is_node_valid = detail::traits::node<T>::is_valid;
    template <typename T>
    concept is_node_invalid = !detail::traits::node<T>::is_valid;
}

namespace nil::gate::errors
{
    template <typename T>
    struct Errors
    {
        using traits = nil::gate::detail::traits::node<T>;
        Error arg_asyncs = Check<traits::arg_async::is_valid>();
        Error arg_core = Check<traits::arg_core::is_valid>();
        Error inputs = Check<traits::inputs::is_valid>();
        Error sync_outputs = Check<traits::sync_outputs::is_valid>();
        Error async_outputs = Check<traits::async_outputs::is_valid>();
    };
}

namespace nil::gate
{
    class Core final
    {
        template <typename T>
        using inputs_t = detail::traits::node<T>::inputs::edges;
        template <typename T>
        using outputs_t = detail::traits::node<T>::outputs::edges;

    public:
        Core() = default;
        ~Core() noexcept = default;

        Core(Core&&) noexcept = delete;
        Core& operator=(Core&&) noexcept = delete;

        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        /// starting from this point - node

        template <concepts::is_node_invalid T>
        outputs_t<T> node(T instance, errors::Errors<T> = errors::Errors<T>());
        template <concepts::is_node_invalid T>
        outputs_t<T> node(T instance, inputs_t<T> edges, errors::Errors<T> = errors::Errors<T>());

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size > 0)
        outputs_t<T> node(T instance, inputs_t<T> input_edges)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                this,
                std::move(input_edges),
                std::move(instance)
            );
            auto* r = n.get();
            owned_nodes.emplace_back(std::move(n));
            return r->output_edges();
        }

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size == 0)
        outputs_t<T> node(T instance)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                this,
                inputs_t<T>(),
                std::move(instance)
            );
            auto* r = n.get();
            owned_nodes.emplace_back(std::move(n));
            return r->output_edges();
        }

        /// starting from this point - edge

        template <typename T>
        auto* edge()
        {
            using type = traits::edgify_t<std::decay_t<T>>;
            auto e = std::make_unique<detail::edges::Data<type>>();
            e->attach(diffs.get());
            auto r = independent_edges.emplace_back(std::move(e)).get();
            return static_cast<edges::Mutable<type>*>(r);
        }

        template <typename T>
        auto* edge(T value)
        {
            using type = traits::edgify_t<std::decay_t<T>>;
            auto e = std::make_unique<detail::edges::Data<type>>(std::move(value));
            e->attach(diffs.get());
            auto r = independent_edges.emplace_back(std::move(e)).get();
            return static_cast<edges::Mutable<type>*>(r);
        }

        template <typename... T>
        Batch<T...> batch(edges::Mutable<T>*... edges) const
        {
            return Batch<T...>(diffs.get(), {edges...});
        }

        template <typename... T>
        Batch<T...> batch(std::tuple<edges::Mutable<T>*...> edges) const
        {
            return Batch<T...>(diffs.get(), edges);
        }

        void commit() const
        {
            runner->run(
                make_callable(
                    [df = diffs->flush()]()
                    {
                        for (const auto& d : df)
                        {
                            if (d)
                            {
                                d->call();
                            }
                        }
                    }
                ),
                owned_nodes
            );
        }

        template <typename Runner, typename... Args>
        void set_runner(Args&&... args) noexcept
        {
            runner = std::make_unique<Runner>(std::forward<Args>(args)...);
        }

        void clear()
        {
            independent_edges.clear();
            owned_nodes.clear();
            diffs = std::make_unique<Diffs>();
        }

    private:
        std::unique_ptr<Diffs> diffs = std::make_unique<Diffs>();
        std::vector<std::unique_ptr<INode>> owned_nodes;
        std::vector<std::unique_ptr<IEdge>> independent_edges;
        std::unique_ptr<IRunner> runner = std::make_unique<runners::Immediate>();
    };
}
