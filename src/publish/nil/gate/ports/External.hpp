#pragma once

#include "../detail/Port.hpp"

#include <functional>

namespace nil::gate
{
    class Core;

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
        External(Core* init_core, T intial_value)
            : core(init_core)
            , port(intial_value)
        {
        }

        explicit External(Core* init_core)
            : core(init_core)
            , port()
        {
        }

        ~External() noexcept final = default;

        External(External&&) = delete;
        External(const External&) = delete;
        External& operator=(External&&) = delete;
        External& operator=(const External&) = delete;

        void set_value(T new_data);

        void unset_value();

        void update(std::function<T(const T*)> callable);

        ports::Mutable<T>* to_direct()
        {
            return &port;
        }

    private:
        Core* core;
        detail::Port<T> port;
    };
}
