#pragma once

#include "../ICallable.hpp"

#include "../detail/DataEdge.hpp"

namespace nil::gate::edges
{
    template <typename T>
    class Batch final
    {
    public:
        Batch() = delete;

        Batch(
            detail::edges::Data<T>* init_edge,
            std::vector<std::unique_ptr<ICallable<void()>>>* init_diffs
        )
            : edge(init_edge)
            , diffs(init_diffs)
        {
        }

        ~Batch() noexcept = default;

        Batch(Batch&&) noexcept = delete;
        Batch& operator=(Batch&&) noexcept = delete;

        Batch(const Batch&) = default;
        Batch& operator=(const Batch&) = default;

        const T& value() const
        {
            return edge->value();
        }

        void set_value(T new_data)
        {
            diffs->push_back(make_callable(
                [e = edge, d = std::move(new_data)]() mutable
                {
                    if (!e->is_equal(d))
                    {
                        e->pend();
                        e->exec(std::move(d));
                        e->done();
                    }
                }
            ));
        }

        detail::edges::Data<T>* edge = nullptr;
        std::vector<std::unique_ptr<ICallable<void()>>>* diffs = nullptr;
    };
}
