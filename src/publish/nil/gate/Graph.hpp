#pragma once

#include "errors.hpp"

#include "traits/portify.hpp"

#include "detail/Node.hpp"
#include "detail/UNode.hpp"
#include "detail/traits/node.hpp"
#include "ports/External.hpp"

#include <span>

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
    class Graph
    {
        friend class Core;
        template <typename T>
        using inputs_t = detail::traits::node<T>::inputs::ports;

        explicit Graph(Core* init_core)
            : core(init_core)
        {
        }

    public:
        ~Graph() noexcept
        {
            clear();
        }

        Graph(Graph&&) = default;
        Graph(const Graph&) = default;
        Graph& operator=(Graph&&) = default;
        Graph& operator=(const Graph&) = default;

        /// starting from this point - node

        template <concepts::is_node_invalid T>
        void* node(T instance, errors::Node<T> = errors::Node<T>());
        template <concepts::is_node_invalid T>
        void* node(T instance, inputs_t<T> ports, errors::Node<T> = errors::Node<T>());

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size > 0)
        auto* node(T instance, inputs_t<T> input_ports)
        {
            need_to_sort = true;
            auto n = std::make_unique<detail::Node<T>>(
                core,
                std::move(instance),
                std::move(input_ports)
            );
            return static_cast<typename detail::Node<T>::base_t*>(
                owned_nodes.emplace_back(n.release())
            );
        }

        template <concepts::is_node_valid T>
            requires(detail::traits::node<T>::inputs::size == 0)
        auto* node(T instance)
        {
            need_to_sort = true;
            auto n = std::make_unique<detail::Node<T>>(core, std::move(instance), inputs_t<T>());
            return static_cast<typename detail::Node<T>::base_t*>(
                owned_nodes.emplace_back(n.release())
            );
        }

        template <typename T>
        auto unode(UNode<T>::Info info)
        {
            need_to_sort = true;
            auto n = std::make_unique<UNode<T>>(core, std::move(info));
            return static_cast<UNode<T>*>(owned_nodes.emplace_back(n.release()))->outputs();
        }

        /// starting from this point - link

        template <typename TO, typename FROM>
        auto link(ports::ReadOnly<FROM>* from, ports::External<TO>* to)
            -> nil::gate::Node<nil::xalt::tlist<FROM>, nil::xalt::tlist<>>*;

        /// starting from this point - port

        template <concepts::is_port_invalid T>
        auto* port(errors::Port<T> = errors::Port<T>());
        template <concepts::is_port_invalid T>
        auto* port(T, errors::Port<T> = errors::Port<T>());

        template <concepts::is_port_valid T>
        auto* port()
        {
            auto* p = new ports::External<traits::portify_t<T>>(core); // NOLINT
            external_ports.emplace_back(p);
            return p;
        }

        template <concepts::is_port_valid T>
        auto* port(T value)
        {
            auto* p = new ports::External<traits::portify_t<T>>(core, std::move(value)); // NOLINT
            external_ports.emplace_back(p);
            return p;
        }

        /// starting from this point - misc

        void remove(INode* node)
        {
            remove(owned_nodes, node);
        }

        void remove(EPort* port)
        {
            remove(external_ports, port);
        }

        void clear()
        {
            for (auto* e : external_ports)
            {
                delete e; // NOLINT
            }
            external_ports.clear();

            for (auto* n : owned_nodes)
            {
                delete n; // NOLINT
            }
            owned_nodes.clear();
        }

    private:
        Core* core;
        std::vector<INode*> owned_nodes;
        std::vector<EPort*> external_ports;
        bool need_to_sort = true;

        auto sort() -> std::span<INode* const>
        {
            if (need_to_sort)
            {
                need_to_sort = false;
                std::sort(
                    owned_nodes.begin(),
                    owned_nodes.end(),
                    [](const auto* l, const auto* r) { return l->score() < r->score(); }
                );
            }

            return owned_nodes;
        }

        template <typename T>
        void remove(std::vector<T*>& container, T* ptr)
        {
            if (0 < std::erase_if(
                    container,
                    [ptr](auto* p)
                    {
                        if (p != ptr)
                        {
                            return false;
                        }

                        delete p; // NOLINT
                        return true;
                    }
                ))
            {
                need_to_sort = true;
            }
        }
    };
}
