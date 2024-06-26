#pragma once

#include "Batch.hpp"
#include "Diffs.hpp"
#include "errors.hpp"
#include "runners/immediate.hpp"

#include "edges/Mutable.hpp"
#include "traits/edgify.hpp"

#include "detail/Node.hpp"
#include "detail/traits/node.hpp"

#ifdef NIL_GATE_CHECKS
#include <cassert>
#endif

namespace nil::gate::concepts
{
    template <typename T>
    concept is_node_invalid = !detail::traits::node<T>::is_valid;

    template <typename T>
    concept has_input_has_async                      //
        = detail::traits::node<T>::is_valid          //
        && detail::traits::node<T>::inputs::size > 0 //
        && detail::traits::node<T>::has_async;
    template <typename T>
    concept has_input_no_async                       //
        = detail::traits::node<T>::is_valid          //
        && detail::traits::node<T>::inputs::size > 0 //
        && !detail::traits::node<T>::has_async;
    template <typename T>
    concept no_input_has_async                        //
        = detail::traits::node<T>::is_valid           //
        && detail::traits::node<T>::inputs::size == 0 //
        && detail::traits::node<T>::has_async;
    template <typename T>
    concept no_input_no_async                         //
        = detail::traits::node<T>::is_valid           //
        && detail::traits::node<T>::inputs::size == 0 //
        && !detail::traits::node<T>::has_async;
}

namespace nil::gate
{
    namespace errors
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

    //  TODO:
    //      Figure out what is the best way to make this movable so that
    //  users are not forced to use std::unique_ptr when returning
    //  from method/functions.
    //
    //      The current approach is to edges and nodes a reference to the diffs
    //  which is owned by Core. if Core is moved, diffs object is moved too.
    //      Each node also has a reference to Core. if Core is moved, the pointer
    //  held by the Node becomes invalidated and needs to be reset to the
    //  new Core.
    //
    //      A possible approach is to simply have a context object that owns the
    //  Diffs and a reference to Core. Core will not be copy-able (due to diffs's mutex)
    //  but at least nodes/edges will not need to have more convoluted setup.
    //
    //
    class Core final
    {
        template <typename T>
        using inputs_t = detail::traits::node<T>::inputs::edges;
        template <typename T>
        using outputs_t = detail::traits::node<T>::outputs::edges;
        template <typename T>
        using asyncs_t = detail::traits::node<T>::async_outputs::tuple;

    public:
        Core() = default;
        ~Core() noexcept = default;

        Core(Core&&) noexcept = delete;
        Core& operator=(Core&&) noexcept = delete;

        Core(const Core&) = delete;
        Core& operator=(const Core&) = delete;

        /// starting from this point - node

        template <concepts::is_node_invalid T>
        outputs_t<T> node(
            T instance,
            errors::Errors<T> = errors::Errors<T>() //
        );
        template <concepts::is_node_invalid T>
        outputs_t<T> node(
            T instance,
            inputs_t<T> edges,
            errors::Errors<T> = errors::Errors<T>() //
        );
        template <concepts::is_node_invalid T>
        outputs_t<T> node(
            T instance,
            asyncs_t<T> async_init,
            inputs_t<T> edges,
            errors::Errors<T> = errors::Errors<T>()
        );
        template <concepts::is_node_invalid T>
        outputs_t<T> node(
            T instance,
            asyncs_t<T> async_init,
            errors::Errors<T> = errors::Errors<T>() //
        );

        template <concepts::has_input_no_async T>
        outputs_t<T> node(T instance, inputs_t<T> input_edges)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                this,
                input_edges,
                asyncs_t<T>(),
                std::move(instance)
            );
            auto r = owned_nodes.emplace_back(std::move(n)).get();
            return static_cast<detail::Node<T>*>(r)->output_edges();
        }

        template <concepts::no_input_no_async T>
        outputs_t<T> node(T instance)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                this,
                inputs_t<T>(),
                asyncs_t<T>(),
                std::move(instance)
            );
            auto r = owned_nodes.emplace_back(std::move(n)).get();
            return static_cast<detail::Node<T>*>(r)->output_edges();
        }

        template <concepts::has_input_has_async T>
        outputs_t<T> node(T instance, asyncs_t<T> async_init, inputs_t<T> input_edges)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                this,
                input_edges,
                std::move(async_init),
                std::move(instance)
            );
            auto r = owned_nodes.emplace_back(std::move(n)).get();
            return static_cast<detail::Node<T>*>(r)->output_edges();
        }

        template <concepts::no_input_has_async T>
        outputs_t<T> node(T instance, asyncs_t<T> async_init)
        {
            auto n = std::make_unique<detail::Node<T>>(
                diffs.get(),
                this,
                inputs_t<T>(),
                std::move(async_init),
                std::move(instance)
            );
            auto r = owned_nodes.emplace_back(std::move(n)).get();
            return static_cast<detail::Node<T>*>(r)->output_edges();
        }

        /// starting from this point - edge

        template <typename T>
            requires std::is_same_v<T, std::decay_t<T>>
        auto* edge(T value)
        {
            using type = traits::edgify_t<std::decay_t<T>>;
            auto e = std::make_unique<detail::edges::Data<type>>(diffs.get(), std::move(value));
            auto r = required_edges.emplace_back(std::move(e)).get();
            return static_cast<edges::Mutable<type>*>(r);
        }

        template <typename... T>
        Batch<T...> batch(edges::Mutable<T>*... edges) const
        {
            return Batch<T...>(this, diffs.get(), commit_cb.get(), {edges...});
        }

        template <typename... T>
        Batch<T...> batch(std::tuple<edges::Mutable<T>*...> edges) const
        {
            return Batch<T...>(this, diffs.get(), commit_cb.get(), edges);
        }

        void commit() const
        {
            if (commit_cb)
            {
                commit_cb->call(this);
            }
        }

        template <typename CB>
        void set_commit(CB cb) noexcept
        {
            struct Callable: ICallable<void(const Core*)>
            {
                explicit Callable(CB init_cb)
                    : cb(std::move(init_cb))
                {
                }

                void call(const Core* core) override
                {
                    cb(*core);
                }

                CB cb;
            };

            commit_cb = std::make_unique<Callable>(std::move(cb));
        }

        void run() const
        {
#ifdef NIL_GATE_CHECKS
            assert(nullptr != diffs);
#endif
            runner->flush(diffs->flush());
            runner->run(owned_nodes);
        }

        void set_runner(std::unique_ptr<IRunner> new_runner) noexcept
        {
            runner = std::move(new_runner);
        }

        void clear()
        {
            required_edges.clear();
            owned_nodes.clear();
            diffs = std::make_unique<Diffs>();
        }

    private:
        std::unique_ptr<Diffs> diffs = std::make_unique<Diffs>();
        std::vector<std::unique_ptr<INode>> owned_nodes;
        std::vector<std::unique_ptr<IEdge>> required_edges;
        std::unique_ptr<ICallable<void(const Core*)>> commit_cb;
        std::unique_ptr<IRunner> runner = std::make_unique<runners::Immediate>();
    };
}
