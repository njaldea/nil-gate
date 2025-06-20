#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "../ports/Mutable.hpp"
#include "../traits/compare.hpp"
#include "../traits/compatibility.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace nil::gate::detail::ports
{
    /**
     * @brief Port type returned by Core::port.
     *  For internal use.
     *
     * @tparam T
     */
    template <typename T>
    class Data final: public gate::ports::Mutable<T>
    {
    public:
        Data()
            : state(EState::Pending)
            , data(std::nullopt)
            , diffs(nullptr)
        {
        }

        explicit Data(T init_data)
            : state(EState::Stale)
            , data(std::make_optional<T>(std::move(init_data)))
            , diffs(nullptr)
        {
        }

        ~Data() noexcept override = default;

        Data(Data&&) noexcept = delete;
        Data& operator=(Data&&) noexcept = delete;

        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

        const T& value() const override
        {
            return *data;
        }

        void set_value(T new_data) override
        {
            diffs->push(make_callable(
                [this, new_data = std::move(new_data)]() mutable
                {
                    if (!is_equal(new_data))
                    {
                        pend();
                        exec(std::move(new_data));
                        done();
                    }
                }
            ));
        }

        bool is_equal(const T& value) const
        {
            return data.has_value() && traits::compare<T>::match(data.value(), value);
        }

        void exec(T new_data)
        {
            data.emplace(std::move(new_data));

            for (auto& a : adapters)
            {
                a.second->set(*data);
            }

            for (auto* n : this->node_out)
            {
                n->input_changed();
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

        void done()
        {
            state = EState::Stale;
        }

        void attach(INode* node)
        {
            node_out.push_back(node);
        }

        void attach(Diffs* new_diffs)
        {
            diffs = new_diffs;
        }

        bool is_ready() const
        {
            return state != EState::Pending && data.has_value();
        }

        template <typename U>
            requires(!std::same_as<T, U> && !concepts::compatibility_requires_cache<U, T>)
        auto* adapt()
        {
            using FROM = T;
            using TO = U;

            struct Impl final: IAdapter
            {
                explicit Impl(detail::ports::Data<FROM>* init_port)
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
                explicit Impl(detail::ports::Data<FROM>* init_port)
                    : IAdapter(init_port)
                {
                    if (this->port->state == EState::Stale)
                    {
                        set(this->port->value());
                    }
                }

                const TO& value() const
                {
                    return cache.value();
                }

                void set(const FROM& new_value) override
                {
                    cache = traits::compatibility<TO, FROM>::convert(new_value);
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
        std::vector<INode*> node_out;

        struct IAdapter
        {
            explicit IAdapter(detail::ports::Data<T>* init_port)
                : port(init_port)
            {
            }

            virtual ~IAdapter() noexcept = default;

            IAdapter(const IAdapter&) = delete;
            IAdapter(IAdapter&&) = delete;
            IAdapter& operator=(const IAdapter&) = delete;
            IAdapter& operator=(IAdapter&&) = delete;

            void attach(INode* node)
            {
                port->attach(node);
            }

            bool is_ready() const
            {
                return port->is_ready();
            }

            virtual void set(const T&) = 0;

            detail::ports::Data<T>* port;
        };

        std::unordered_map<const void*, std::unique_ptr<IAdapter>> adapters;
    };
}
