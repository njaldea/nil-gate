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
    concept is_node_invalid = !is_node_valid<T>;

    template <typename T>
    concept is_edge_valid =                //
        std::is_same_v<T, std::decay_t<T>> //
        && traits::is_edge_type_valid_v<traits::edgify_t<T>>;
    template <typename T>
    concept is_edge_invalid = !is_edge_valid<T>;
}

namespace nil::gate::errors
{
    template <typename T>
    struct Node
    {
        using traits = detail::traits::node<T>;
        // clang-format off
        Error arg_asyncs = Check<traits::arg_async::is_valid>("invalid async arg type detected, must be by copy or by const ref");
        Error arg_core = Check<traits::arg_core::is_valid>("invalid core type, must be `const Core&`");
        Error inputs = Check<traits::inputs::is_valid>("invalid input type detected");
        Error sync_outputs = Check<traits::sync_outputs::is_valid>("invalid sync output type detected");
        Error async_outputs = Check<traits::async_outputs::is_valid>("invalid async output type detected");
        // clang-format on
    };

    template <typename T>
    struct Edge
    {
        static constexpr auto decayed_v = std::is_same_v<T, std::decay_t<T>>;
        static constexpr auto valid_type = traits::is_edge_type_valid_v<traits::edgify_t<T>>;
        Error not_decayed = Check<decayed_v>("Type provided is not decayed");
        Error invalid_type = Check<valid_type>("Edge type is not allowed");
    };
}

namespace nil::gate
{
    class Core final
    {
        template <typename T>
        using inputs_t = detail::traits::node<T>::inputs::edges;
        template <typename T>
        using outputs_t = std::conditional_t<
            detail::traits::node<T>::outputs::size == 0,
            void,
            typename detail::traits::node<T>::outputs::edges>;

    public:
        Core() = default;

        ~Core() noexcept
        {
            clear();
        }

        Core(Core&&) noexcept = delete;
        Core& operator=(Core&&) noexcept = delete;

        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        /// starting from this point - node

        template <concepts::is_node_invalid T>
        outputs_t<T> node(T instance, errors::Node<T> = errors::Node<T>());
        template <concepts::is_node_invalid T>
        outputs_t<T> node(T instance, inputs_t<T> edges, errors::Node<T> = errors::Node<T>());

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size > 0)
        outputs_t<T> node(T instance, inputs_t<T> input_edges)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                std::move(instance),
                std::move(input_edges)
            );
            return static_cast<detail::Node<T>*>(owned_nodes.emplace_back(n.release()))
                ->output_edges();
        }

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size == 0)
        outputs_t<T> node(T instance)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                std::move(instance),
                inputs_t<T>()
            );
            return static_cast<detail::Node<T>*>(owned_nodes.emplace_back(n.release()))
                ->output_edges();
        }

        /// starting from this point - link

        template <typename TO, typename FROM>
            requires requires(TO to, FROM from) {
                { traits::compatibility<TO, FROM>::convert(to) };
            }
        void link(edges::ReadOnly<FROM>* from, edges::Mutable<TO>* to)
        {
            this->node([to](const TO& v) { to->set_value(v); }, {from});
        }

        /// starting from this point - edge

        template <concepts::is_edge_invalid T>
        auto* edge(errors::Edge<T> = errors::Edge<T>());
        template <concepts::is_edge_invalid T>
        auto* edge(T, errors::Edge<T> = errors::Edge<T>());

        template <concepts::is_edge_valid T>
        auto* edge()
        {
            using type = traits::edgify_t<T>;
            auto e = std::make_unique<detail::edges::Data<traits::edgify_t<T>>>();
            e->attach(diffs.get());
            auto* r = independent_edges.emplace_back(e.release());
            return static_cast<edges::Mutable<type>*>(r);
        }

        template <concepts::is_edge_valid T>
        auto* edge(T value)
        {
            using type = traits::edgify_t<T>;
            auto e = std::make_unique<detail::edges::Data<type>>(std::move(value));
            e->attach(diffs.get());
            auto* r = independent_edges.emplace_back(e.release());
            return static_cast<edges::Mutable<type>*>(r);
        }

        /// starting from this point - batch

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

        /// starting from this point - misc

        /**
         * Commit all of the changes to the graph.
         * Changes are scheduled by mutating the edges through their set_value method.
         */
        void commit() const
        {
            runner->run(
                self,
                [this]()
                {
                    for (const auto& d : diffs->flush())
                    {
                        if (d)
                        {
                            d->call();
                        }
                    }
                },
                owned_nodes
            );
        }

        /**
         * Use a custom runner. By default uses `nil::gate:runner::Immediate`.
         */
        template <typename Runner, typename... Args>
        void set_runner(Args&&... args) noexcept
        {
            runner = std::make_unique<Runner>(std::forward<Args>(args)...);
        }

        void clear()
        {
            for (auto* e : independent_edges)
            {
                delete e; // NOLINT
            }
            independent_edges.clear();

            for (auto* e : owned_nodes)
            {
                delete e; // NOLINT
            }
            owned_nodes.clear();
            diffs = std::make_unique<Diffs>();
        }

    private:
        Core* self = this; // This is to be used for commit since executing the runner requires a
                           // non-const version of this. This is done this way to only allow users
                           // to add nodes/edges on the non-const version while still allowing them
                           // to commit on the const version.
        std::unique_ptr<Diffs> diffs = std::make_unique<Diffs>();
        std::vector<INode*> owned_nodes;
        std::vector<IEdge*> independent_edges;
        std::unique_ptr<IRunner> runner = std::make_unique<runners::Immediate>();
    };
}
