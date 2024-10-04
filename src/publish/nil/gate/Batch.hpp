#pragma once

#include "Diffs.hpp"
#include "ICallable.hpp"

#include "detail/DataEdge.hpp"
#include "edges/Batch.hpp"

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
        Batch(Diffs* init_diffs, const std::tuple<edges::Mutable<T>*...>& init_edges)
            : diffs(init_diffs)
            , edges(transform_edges(init_edges, std::make_index_sequence<sizeof...(T)>()))
        {
        }

        ~Batch() noexcept
        {
            if (!batch_diffs.empty())
            {
                diffs->push_batch(std::move(batch_diffs));
            }
        }

        Batch(Batch&&) noexcept = delete;
        Batch& operator=(Batch&&) noexcept = delete;

        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;

        template <std::size_t index>
        friend auto* get(Batch& batch)
        {
            return std::addressof(std::get<index>(batch.edges));
        }

        template <std::size_t index>
        friend auto* get(Batch&& batch)
        {
            return std::addressof(std::get<index>(batch.edges));
        }

    private:
        template <std::size_t... I>
        std::tuple<edges::Batch<T>...> transform_edges(
            const std::tuple<edges::Mutable<T>*...>& init_edges,
            std::index_sequence<I...> /* indexes */
        )
        {
            return {transform_edge(std::get<I>(init_edges))...};
        }

        template <typename U>
        edges::Batch<U> transform_edge(edges::Mutable<U>* init_edge)
        {
            return edges::Batch(static_cast<detail::edges::Data<U>*>(init_edge), &batch_diffs);
        }

        std::vector<std::unique_ptr<ICallable<void()>>> batch_diffs;
        Diffs* diffs = nullptr;
        std::tuple<edges::Batch<T>...> edges;
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
