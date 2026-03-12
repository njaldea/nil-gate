#pragma once

#include "../detail/Port.hpp"

namespace nil::gate
{
    class EPort
    {
    public:
        EPort() = default;
        virtual ~EPort() noexcept = default;

        EPort(EPort&&) noexcept = delete;
        EPort& operator=(EPort&&) noexcept = delete;

        EPort(const EPort&) = delete;
        EPort& operator=(const EPort&) = delete;
    };
}

namespace nil::gate::ports
{
    template <typename T>
    class External final: public EPort
    {
    public:
        explicit External(T intial_value)
            : port(std::move(intial_value))
        {
        }

        explicit External() = default;

        ~External() noexcept final = default;

        External(External&&) = delete;
        External(const External&) = delete;
        External& operator=(External&&) = delete;
        External& operator=(const External&) = delete;

        ports::Mutable<T>* to_direct()
        {
            return &port;
        }

    private:
        detail::Port<T> port;
    };
}
