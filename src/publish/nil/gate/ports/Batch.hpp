#pragma once

#include "../ICallable.hpp"

#include "../detail/DataPort.hpp"

namespace nil::gate::ports
{
    template <typename T>
    class Batch final
    {
    public:
        Batch() = delete;

        Batch(
            detail::ports::Data<T>* init_port,
            std::vector<std::unique_ptr<ICallable<void()>>>* init_diffs
        )
            : port(init_port)
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
            return port->value();
        }

        void set_value(T new_data)
        {
            diffs->push_back(make_callable(
                [e = port, d = std::move(new_data)]() mutable
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

        detail::ports::Data<T>* port = nullptr;
        std::vector<std::unique_ptr<ICallable<void()>>>* diffs = nullptr;
    };
}
