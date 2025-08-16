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
            , ports(std::apply(
                  [this](ports::Mutable<T>*... port) -> std::tuple<ports::Batch<T>...>
                  { return {ports::Batch(static_cast<detail::Port<T>*>(port), &batch_diffs)...}; },
                  init_ports
              ))
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
