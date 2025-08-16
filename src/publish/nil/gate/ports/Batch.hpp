#pragma once

#include "../ICallable.hpp"

#include "../detail/Port.hpp"

namespace nil::gate::ports
{
    template <typename T>
    class Batch final
    {
    public:
        Batch() = delete;

        Batch(
            detail::Port<T>* init_port,
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

        const T& value() const noexcept
        {
            return port->value();
        }

        bool has_value() const noexcept
        {
            return port->has_value();
        }

        /**
         * @brief Set the value object. Effect is deferred. To be applied on next core.commit()
         *
         * @param new_data
         */
        void set_value(T new_data)
        {
            diffs->push_back(make_callable(
                [e = port, d = std::move(new_data)]() mutable
                {
                    if (!e->is_equal(d))
                    {
                        e->pend();
                        e->set(std::move(d));
                        e->done();
                    }
                }
            ));
        }

        /**
         * @brief Unset the value. Effect is deferred. To be applied on next core.commit()
         */
        void unset_value()
        {
            diffs->push_back(make_callable(
                [e = port]() mutable
                {
                    if (e->has_value())
                    {
                        e->pend();
                        e->unset();
                        e->done();
                    }
                }
            ));
        }

    private:
        detail::Port<T>* port = nullptr;
        std::vector<std::unique_ptr<ICallable<void()>>>* diffs = nullptr;
    };
}
