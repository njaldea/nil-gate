#pragma once

#include "../detail/Port.hpp"
#include "ReadOnly.hpp"

namespace nil::gate
{
    class Core;
}

namespace nil::gate::ports
{
    template <typename T>
    class External: public ports::ReadOnly<T>
    {
    public:
        External(Core* init_core, detail::Port<T>* init_port)
            : core(init_core)
            , port(init_port)
        {
        }

        ~External() noexcept
        {
            delete port;
        }

        External(External&&) = delete;
        External(const External&) = delete;
        External& operator=(External&&) = delete;
        External& operator=(const External&) = delete;

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
