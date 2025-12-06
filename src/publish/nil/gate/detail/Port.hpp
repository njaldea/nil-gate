#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "../ports/Mutable.hpp"
#include "../traits/compatibility.hpp"
#include "../traits/port_override.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace nil::gate::detail
{
    /**
     * @brief Port type returned by Core::port.
     *  For internal use.
     *
     * @tparam T
     */
    template <typename T>
    class Port final: public gate::ports::Mutable<T>
    {
    public:
        explicit Port()
            : state(EState::Pending)
            , data(std::nullopt)
            , diffs(nullptr)
            , parent(nullptr)
        {
        }

        explicit Port(T init_data)
            : state(EState::Stale)
            , data(std::make_optional<T>(std::move(init_data)))
            , diffs(nullptr)
            , parent(nullptr)
        {
        }

        ~Port() noexcept override
        {
            for (auto* node : node_out)
            {
                node->detach_in(this);
            }
        }

        Port(Port&&) noexcept = delete;
        Port& operator=(Port&&) noexcept = delete;

        Port(const Port&) = delete;
        Port& operator=(const Port&) = delete;

        int score() const noexcept override
        {
            return parent != nullptr ? parent->score() : 0;
        }

        const T& value() const noexcept override
        {
            return *data;
        }

        bool has_value() const noexcept override
        {
            return data.has_value() && nil::gate::traits::port::has_value(*data);
        }

        void set_value(T new_data) override
        {
            diffs->push(make_callable(
                [this, new_data = std::move(new_data)]() mutable
                {
                    if (!is_equal(new_data))
                    {
                        pend();
                        set(std::move(new_data));
                        done();
                    }
                }
            ));
        }

        void unset_value() override
        {
            diffs->push(make_callable(
                [this]() mutable
                {
                    if (has_value())
                    {
                        pend();
                        unset();
                        done();
                    }
                }
            ));
        }

        bool is_equal(const T& value) const
        {
            return has_value() && nil::gate::traits::port::is_eq(data.value(), value);
        }

        void set(T&& new_data)
        {
            if (!is_equal(new_data))
            {
                if (data.has_value())
                {
                    *data = std::move(new_data);
                }
                else
                {
                    data.emplace(std::move(new_data));
                }

                for (auto& a : adapters)
                {
                    a.second->set(*data);
                }

                for (auto* n : this->node_out)
                {
                    n->input_changed();
                }
            }
        }

        void unset()
        {
            if (has_value())
            {
                nil::gate::traits::port::unset(data);

                for (auto& a : adapters)
                {
                    a.second->unset();
                }
                for (auto* n : this->node_out)
                {
                    n->input_changed();
                }
            }
        }

        void pend()
        {
            if (state != EState::Pending)
            {
                state = EState::Pending;
                for (auto* n : this->node_out)
                {
                    n->pend();
                }
            }
        }

        // split to allow finalization to be done
        // in the main thread if using parallel runner
        void done()
        {
            state = EState::Stale;
        }

        void attach_in(INode* node)
        {
            parent = node;
        }

        void attach_out(INode* node)
        {
            node_out.push_back(node);
        }

        void detach_in(INode* node)
        {
            std::erase_if(node_out, [node](auto* n) { return n == node; });
        }

        void detach_out(INode* node)
        {
            std::erase_if(node_out, [node](auto* n) { return n == node; });
        }

        void attach(Diffs* new_diffs)
        {
            diffs = new_diffs;
        }

        bool is_ready() const
        {
            return state != EState::Pending && has_value();
        }

        template <typename U>
            requires(!std::same_as<T, U> && !concepts::compatibility_requires_cache<U, T>)
        auto* adapt()
        {
            using FROM = T;
            using TO = U;

            struct Impl final: IAdapter
            {
                explicit Impl(Port<FROM>* init_port)
                    : IAdapter(init_port)
                    , cache(nullptr)
                {
                    if (this->port->state == EState::Stale)
                    {
                        set(this->port->value());
                    }
                }

                const TO& value() const
                {
                    return *cache;
                }

                void set(const FROM& new_value) override
                {
                    cache = &traits::compatibility<TO, FROM>::convert(new_value);
                }

                void unset() override
                {
                    cache = nullptr;
                }

                const TO* cache;
            };

            const void* const id = (void*)traits::compatibility<TO, FROM>::convert;
            if (auto it = adapters.find(id); it != adapters.end())
            {
                return static_cast<Impl*>(it->second.get());
            }
            return static_cast<Impl*>(
                adapters.emplace(id, std::make_unique<Impl>(this)).first->second.get()
            );
        }

        template <typename U>
            requires(!std::same_as<T, U> && nil::gate::concepts::compatibility_requires_cache<U, T>)
        auto* adapt()
        {
            using FROM = T;
            using TO = U;

            struct Impl final: IAdapter
            {
                explicit Impl(Port<FROM>* init_port)
                    : IAdapter(init_port)
                {
                    if (this->port->state == EState::Stale)
                    {
                        set(this->port->value());
                    }
                }

                const TO& value() const
                {
                    // set should be called before this
                    // so cache should have been available
                    return cache.value();
                }

                void set(const FROM& new_value) override
                {
                    cache = traits::compatibility<TO, FROM>::convert(new_value);
                }

                void unset() override
                {
                    cache = {};
                }

                std::optional<TO> cache;
            };

            const void* const id = (void*)traits::compatibility<TO, FROM>::convert;
            if (auto it = adapters.find(id); it != adapters.end())
            {
                return static_cast<Impl*>(it->second.get());
            }
            return static_cast<Impl*>(
                adapters.emplace(id, std::make_unique<Impl>(this)).first->second.get()
            );
        }

    private:
        enum class EState
        {
            Stale = 0b0001,
            Pending = 0b0010
        };

        EState state;
        std::optional<T> data;
        nil::gate::Diffs* diffs;
        INode* parent;
        std::vector<INode*> node_out;

        struct IAdapter
        {
            explicit IAdapter(Port<T>* init_port)
                : port(init_port)
            {
            }

            virtual ~IAdapter() noexcept = default;

            IAdapter(const IAdapter&) = delete;
            IAdapter(IAdapter&&) = delete;
            IAdapter& operator=(const IAdapter&) = delete;
            IAdapter& operator=(IAdapter&&) = delete;

            void attach_out(INode* node)
            {
                port->attach_out(node);
            }

            bool is_ready() const
            {
                return port->is_ready();
            }

            virtual void set(const T&) = 0;

            virtual void unset() = 0;

            Port<T>* port;
        };

        std::unordered_map<const void*, std::unique_ptr<IAdapter>> adapters;
    };
}
