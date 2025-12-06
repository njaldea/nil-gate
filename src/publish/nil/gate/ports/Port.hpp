#pragma once

#include "../detail/Port.hpp"
#include "ReadOnly.hpp"

namespace nil::gate
{
    class Core;
}

namespace nil::gate
{
    template <typename T>
    class Port: public ports::ReadOnly<T>
    {
    public:
        Port(Core* init_core, detail::Port<T>* init_port)
            : core(init_core)
            , port(init_port)
        {
        }

        ~Port() noexcept
        {
            delete port;
        }

        Port(Port&&) = delete;
        Port(const Port&) = delete;
        Port& operator=(Port&&) = delete;
        Port& operator=(const Port&) = delete;

        void set_value(T new_data);

        void unset_value();

        [[nodiscard]] const T& value() const noexcept override
        {
            return port->value();
        }

        [[nodiscard]] bool has_value() const noexcept override
        {
            return port->has_value();
        }

        ports::ReadOnly<T>* to_compat()
        {
            return port;
        }

    private:
        Core* core;
        detail::Port<T>* port;
    };
}
