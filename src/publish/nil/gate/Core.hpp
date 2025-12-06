#pragma once

#include "Batch.hpp"
#include "Diffs.hpp"
#include "errors.hpp"
#include "runners/Immediate.hpp"

#include "ports/Mutable.hpp"
#include "traits/portify.hpp"

#include "detail/Node.hpp"
#include "detail/UNode.hpp"
#include "detail/traits/node.hpp"

#include <vector>

namespace nil::gate::concepts
{
    template <typename T>
    concept is_node_valid = detail::traits::node<T>::is_valid;
    template <typename T>
    concept is_node_invalid = !is_node_valid<T>;

    template <typename T>
    concept is_port_valid =                //
        std::is_same_v<T, std::decay_t<T>> //
        && traits::is_port_type_valid_v<traits::portify_t<T>>;
    template <typename T>
    concept is_port_invalid = !is_port_valid<T>;
}

namespace nil::gate::errors
{
    template <typename T>
    struct Node
    {
        using traits = detail::traits::node<T>;
        // clang-format off
        Error arg_opt = Check<traits::arg_opt::is_valid>("invalid opt arg type detected, must be by copy or by const ref");
        Error arg_core = Check<traits::arg_core::is_valid>("invalid core type, must be `const Core&`");
        Error inputs = Check<traits::inputs::is_valid>("invalid input type detected");
        Error req_outputs = Check<traits::req_outputs::is_valid>("invalid req output type detected");
        Error opt_outputs = Check<traits::opt_outputs::is_valid>("invalid opt output type detected");
        // clang-format on
    };

    template <typename T>
    struct Port
    {
        static constexpr auto decayed_v = std::is_same_v<T, std::decay_t<T>>;
        static constexpr auto valid_type = traits::is_port_type_valid_v<traits::portify_t<T>>;
        Error not_decayed = Check<decayed_v>("Type provided is not decayed");
        Error invalid_type = Check<valid_type>("Port type is not allowed");
    };
}

namespace nil::gate
{
    class Core final
    {
        template <typename T>
        using inputs_t = detail::traits::node<T>::inputs::ports;
        template <typename T>
        using outputs_t = std::conditional_t<
            detail::traits::node<T>::outputs::size == 0,
            void,
            typename detail::traits::node<T>::outputs::ports>;

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
        outputs_t<T> node(T instance, inputs_t<T> ports, errors::Node<T> = errors::Node<T>());

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size > 0)
        outputs_t<T> node(T instance, inputs_t<T> input_ports)
        {
            auto n = std::make_unique<detail::Node<T>>(
                this,
                diffs.get(),
                std::move(instance),
                std::move(input_ports)
            );
            return static_cast<detail::Node<T>*>(owned_nodes.emplace_back(n.release()))
                ->output_ports();
        }

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size == 0)
        outputs_t<T> node(T instance)
        {
            auto n = std::make_unique<detail::Node<T>>(
                this,
                diffs.get(),
                std::move(instance),
                inputs_t<T>()
            );
            return static_cast<detail::Node<T>*>(owned_nodes.emplace_back(n.release()))
                ->output_ports();
        }

        template <typename T>
        auto unode(UNode<T>::Info info)
        {
            auto n = std::make_unique<UNode<T>>(this, diffs.get(), std::move(info));
            return static_cast<UNode<T>*>(owned_nodes.emplace_back(n.release()))->output_ports();
        }

        /// starting from this point - link

        template <typename TO, typename FROM>
        void link(ports::ReadOnly<FROM>* from, ports::Mutable<TO>* to)
        {
            static_assert(concepts::is_compatible<TO, FROM>, "Not Compatible");
            this->node([to](const TO& v) { to->set_value(v); }, {from});
        }

        /// starting from this point - port

        template <concepts::is_port_invalid T>
        auto* port(errors::Port<T> = errors::Port<T>());
        template <concepts::is_port_invalid T>
        auto* port(T, errors::Port<T> = errors::Port<T>());

        template <concepts::is_port_valid T>
        auto* port()
        {
            using type = traits::portify_t<T>;
            auto e = std::make_unique<detail::Port<traits::portify_t<T>>>();
            e->attach(diffs.get());
            auto* r = independent_ports.emplace_back(e.release());
            return static_cast<ports::Mutable<type>*>(r);
        }

        template <concepts::is_port_valid T>
        auto* port(T value)
        {
            using type = traits::portify_t<T>;
            auto e = std::make_unique<detail::Port<type>>(std::move(value));
            e->attach(diffs.get());
            auto* r = independent_ports.emplace_back(e.release());
            return static_cast<ports::Mutable<type>*>(r);
        }

        /// starting from this point - batch

        template <typename... T>
        Batch<T...> batch(ports::Mutable<T>*... ports) const
        {
            return Batch<T...>(diffs.get(), {ports...});
        }

        template <typename... T>
        Batch<T...> batch(std::tuple<ports::Mutable<T>*...> ports) const
        {
            return Batch<T...>(diffs.get(), ports);
        }

        template <typename... T>
        std::unique_ptr<Batch<T...>> make_batch(std::tuple<ports::Mutable<T>*...> ports) const
        {
            return std::make_unique<Batch<T...>>(diffs.get(), ports);
        }

        template <typename T>
        UBatch<T> ubatch(std::vector<ports::Mutable<T>*> ports) const
        {
            return UBatch<T>(diffs.get(), std::move(ports));
        }

        template <typename T>
        std::unique_ptr<UBatch<T>> make_ubatch(std::vector<ports::Mutable<T>*> ports) const
        {
            return std::make_unique<UBatch<T>>(diffs.get(), std::move(ports));
        }

        /// starting from this point - misc

        /**
         * Commit all of the changes to the graph.
         * Changes are scheduled by mutating the ports through their set_value method.
         */
        void commit() const
        {
            runner->run(
                [this, d = std::shared_ptr(diffs->flush())]() -> std::span<INode* const>
                {
                    if (d)
                    {
                        d->call();
                    }
                    return this->owned_nodes;
                }
            );
        }

        /**
         * Use a custom runner. By default uses `nil::gate:runner::Immediate`.
         */
        template <typename Runner, typename... Args>
        void set_runner(Args&&... args)
        {
            runner.reset();
            runner = std::make_unique<Runner>(std::forward<Args>(args)...);
        }

        /**
         * Use a custom runner. By default uses `nil::gate:runner::Immediate`.
         */
        template <typename Runner>
        void set_runner(Runner&& new_runner)
        {
            runner.reset();
            runner = std::make_unique<Runner>(std::forward<Runner>(new_runner));
        }

        void clear()
        {
            for (auto* e : independent_ports)
            {
                delete e; // NOLINT
            }
            independent_ports.clear();

            for (auto* e : owned_nodes)
            {
                delete e; // NOLINT
            }
            owned_nodes.clear();
            diffs = std::make_unique<Diffs>();
        }

    private:
        std::unique_ptr<Diffs> diffs = std::make_unique<Diffs>();
        std::vector<INode*> owned_nodes;
        std::vector<IPort*> independent_ports;
        std::unique_ptr<IRunner> runner = std::make_unique<runners::Immediate>();
    };
}
