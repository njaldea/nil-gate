#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "../edges/Mutable.hpp"
#include "../traits/compatibility.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace nil::gate::detail::edges
{
    /**
     * @brief Edge type returned by Core::edge.
     *  For internal use.
     *
     * @tparam T
     */
    template <typename T>
    class Data final: public nil::gate::edges::Mutable<T>
    {
    public:
        Data()
            : nil::gate::edges::Mutable<T>()
            , state(EState::Pending)
            , data(std::nullopt)
            , diffs(nullptr)
        {
        }

        explicit Data(T init_data)
            : nil::gate::edges::Mutable<T>()
            , state(EState::Stale)
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
            return data.has_value() && (data.value() == value);
        }

        void exec(T new_data)
        {
            data.emplace(std::move(new_data));

            for (auto& a : adapters)
            {
                a.second->set(*data);
            }
        }

        void pend()
        {
            if (state != EState::Pending)
            {
                state = EState::Pending;
                for (auto* out : this->outs)
                {
                    out->pend();
                }
            }
        }

        void done()
        {
            state = EState::Stale;
        }

        void attach(INode* node)
        {
            outs.push_back(node);
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
        auto* adapt()
        {
            if constexpr (std::is_same_v<T, U>)
            {
                struct Impl final: IAdapter
                {
                    explicit Impl(detail::edges::Data<T>* init_edge)
                        : edge(init_edge)
                    {
                        if (edge->state == EState::Stale)
                        {
                            set(edge->value());
                        }
                    }

                    const T& value() const
                    {
                        return cache.value();
                    }

                    void attach(INode* node)
                    {
                        edge->attach(node);
                    }

                    bool is_ready() const
                    {
                        return edge->is_ready();
                    }

                    void set(const T& new_value)
                    {
                        cache.emplace(std::cref(new_value));
                    }

                    detail::edges::Data<T>* edge;
                    std::optional<std::reference_wrapper<const U>> cache;
                };

                if (auto it = adapters.find(nullptr); it != adapters.end())
                {
                    return static_cast<Impl*>(it->second.get());
                }

                return static_cast<Impl*>(
                    adapters.emplace(nullptr, std::make_unique<Impl>(this)).first->second.get()
                );
            }
            else
            {
                constexpr auto needs_cache = std::is_reference_v<
                    decltype(traits::compatibility<U, T>::convert(std::declval<T>()))>;
                if constexpr (needs_cache)
                {
                    struct Impl final: IAdapter
                    {
                        explicit Impl(detail::edges::Data<T>* init_edge)
                            : edge(init_edge)
                        {
                            if (edge->state == EState::Stale)
                            {
                                set(edge->value());
                            }
                        }

                        const U& value() const
                        {
                            return cache.value();
                        }

                        void attach(INode* node)
                        {
                            edge->attach(node);
                        }

                        bool is_ready() const
                        {
                            return edge->is_ready();
                        }

                        void set(const T& new_value) override
                        {
                            cache.emplace(std::cref(new_value));
                        }

                        detail::edges::Data<T>* edge;
                        std::optional<std::reference_wrapper<const U>> cache;
                    };

                    const void* const id = (void*)traits::compatibility<U, T>::convert;
                    if (auto it = adapters.find(id); it != adapters.end())
                    {
                        return static_cast<Impl*>(it->second.get());
                    }
                    return static_cast<Impl*>(
                        adapters.emplace(id, std::make_unique<Impl>(this)).first->second.get()
                    );
                }
                else
                {
                    struct Impl final: IAdapter
                    {
                        explicit Impl(detail::edges::Data<T>* init_edge)
                            : edge(init_edge)
                        {
                            if (edge->state == EState::Stale)
                            {
                                set(edge->value());
                            }
                        }

                        const U& value() const
                        {
                            return cache.value();
                        }

                        void attach(INode* node)
                        {
                            edge->attach(node);
                        }

                        bool is_ready() const
                        {
                            return edge->is_ready();
                        }

                        void set(const T& new_value) override
                        {
                            cache = traits::compatibility<U, T>::convert(new_value);
                        }

                        detail::edges::Data<T>* edge;
                        std::optional<U> cache;
                    };

                    const void* const id = (void*)traits::compatibility<U, T>::convert;
                    if (auto it = adapters.find(id); it != adapters.end())
                    {
                        return static_cast<Impl*>(it->second.get());
                    }
                    return static_cast<Impl*>(
                        adapters.emplace(id, std::make_unique<Impl>(this)).first->second.get()
                    );
                }
            }
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
        std::vector<INode*> outs;

        struct IAdapter
        {
            IAdapter() = default;
            virtual ~IAdapter() noexcept = default;

            IAdapter(const IAdapter&) = delete;
            IAdapter(IAdapter&&) = delete;
            IAdapter& operator=(const IAdapter&) = delete;
            IAdapter& operator=(IAdapter&&) = delete;

            virtual void set(const T&) = 0;
        };

        std::unordered_map<const void*, std::unique_ptr<IAdapter>> adapters;
    };
}
