#pragma once

#include "Diffs.hpp"
#include "ICallable.hpp"

#include "detail/Port.hpp"
#include "ports/Batch.hpp"

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace nil::gate
{
    template <typename... T>
    class Batch final
    {
    public:
        Batch(Diffs* init_diffs, const std::tuple<ports::Mutable<T>*...>& init_ports)
            : diffs(init_diffs)
            , ports(transform_ports(init_ports, std::index_sequence_for<T...>()))
        {
        }

        ~Batch() noexcept
        {
            if (!batch_diffs.empty())
            {
                diffs->push(make_callable(
                    [ds = std::move(batch_diffs)]()
                    {
                        for (const auto& d : ds)
                        {
                            if (d)
                            {
                                d->call();
                            }
                        }
                    }
                ));
            }
        }

        Batch(Batch&&) noexcept = delete;
        Batch& operator=(Batch&&) noexcept = delete;

        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;

        template <std::size_t index>
        friend auto* get(Batch& batch)
        {
            return std::addressof(std::get<index>(batch.ports));
        }

        template <std::size_t index>
        friend auto* get(Batch&& batch)
        {
            return std::addressof(std::get<index>(batch.ports));
        }

    private:
        template <std::size_t... I>
        std::tuple<ports::Batch<T>...> transform_ports(
            const std::tuple<ports::Mutable<T>*...>& init_ports,
            std::index_sequence<I...> /* indexes */
        )
        {
            return {transform_port(std::get<I>(init_ports))...};
        }

        template <typename U>
        ports::Batch<U> transform_port(ports::Mutable<U>* init_port)
        {
            return ports::Batch(static_cast<detail::Port<U>*>(init_port), &batch_diffs);
        }

        std::vector<std::unique_ptr<ICallable<void()>>> batch_diffs;
        Diffs* diffs = nullptr;
        std::tuple<ports::Batch<T>...> ports;
    };
}

template <typename... T>
struct std::tuple_size<nil::gate::Batch<T...>>: std::integral_constant<std::size_t, sizeof...(T)>
{
};

template <std::size_t I, typename... T>
struct std::tuple_element<I, nil::gate::Batch<T...>>
{
    using type = decltype(get<I>(std::declval<nil::gate::Batch<T...>>()));
};
