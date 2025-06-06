#pragma once

#include "Batch.hpp"
#include "Diffs.hpp"
#include "errors.hpp"
#include "runners/Immediate.hpp"

#include "ports/Mutable.hpp"
#include "traits/portify.hpp"

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
        Error arg_asyncs = Check<traits::arg_async::is_valid>("invalid async arg type detected, must be by copy or by const ref");
        Error arg_core = Check<traits::arg_core::is_valid>("invalid core type, must be `const Core&`");
        Error inputs = Check<traits::inputs::is_valid>("invalid input type detected");
        Error sync_outputs = Check<traits::sync_outputs::is_valid>("invalid sync output type detected");
        Error async_outputs = Check<traits::async_outputs::is_valid>("invalid async output type detected");
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
                diffs.get(),
                std::move(instance),
                inputs_t<T>()
            );
            return static_cast<detail::Node<T>*>(owned_nodes.emplace_back(n.release()))
                ->output_ports();
        }

        /// starting from this point - link

        template <typename TO, typename FROM>
            requires requires(TO to, FROM from) {
                { traits::compatibility<TO, FROM>::convert(to) };
            }
        void link(ports::ReadOnly<FROM>* from, ports::Mutable<TO>* to)
        {
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
            auto e = std::make_unique<detail::ports::Data<traits::portify_t<T>>>();
            e->attach(diffs.get());
            auto* r = independent_ports.emplace_back(e.release());
            return static_cast<ports::Mutable<type>*>(r);
        }

        template <concepts::is_port_valid T>
        auto* port(T value)
        {
            using type = traits::portify_t<T>;
            auto e = std::make_unique<detail::ports::Data<type>>(std::move(value));
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

        /// starting from this point - misc

        /**
         * Commit all of the changes to the graph.
         * Changes are scheduled by mutating the ports through their set_value method.
         */
        void commit() const
        {
            runner->run(
                self,
                [d = std::shared_ptr(diffs->flush())]()
                {
                    if (d)
                    {
                        d->call();
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
            runner.reset();
            runner = std::make_unique<Runner>(std::forward<Args>(args)...);
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
        Core* self = this; // This is to be used for commit since executing the runner requires a
                           // non-const version of this. This is done this way to only allow users
                           // to add nodes/ports on the non-const version while still allowing them
                           // to commit on the const version.
        std::unique_ptr<Diffs> diffs = std::make_unique<Diffs>();
        std::vector<INode*> owned_nodes;
        std::vector<IPort*> independent_ports;
        std::unique_ptr<IRunner> runner = std::make_unique<runners::Immediate>();
    };
}
