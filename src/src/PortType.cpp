#include "PortType.hpp"

namespace nil::gate::c
{
    PortType::PortType(
        void* init_value,
        int (*init_eq)(const void*, const void*),
        void (*init_destroy)(void*)
    )
        : value(init_value)
        , eq(init_eq)
        , destroy(init_destroy)
    {
    }

    PortType::PortType(PortType&& o) noexcept
    {
        *this = std::move(o);
    }

    PortType& PortType::operator=(PortType&& o) noexcept
    {
        if (this == &o)
        {
            return *this;
        }

        if (value != nullptr)
        {
            destroy(value);
        }

        eq = o.eq;
        destroy = o.destroy;
        value = o.value;
        o.value = nullptr;
        return *this;
    }

    PortType::~PortType()
    {
        if (value != nullptr)
        {
            destroy(value);
            value = nullptr;
        }
    }
}

namespace nil::gate::traits::port
{
    template <>
    bool is_eq(const c::PortType& current_value, const c::PortType& new_value)
    {
        if (current_value.eq != new_value.eq || current_value.destroy != new_value.destroy)
        {
            return false;
        }
        return 0 != current_value.eq(current_value.value, new_value.value);
    }

    template <>
    bool has_value(const c::PortType& value)
    {
        return value.value != nullptr;
    }

    template <>
    void unset(std::optional<c::PortType>& value)
    {
        if (value.has_value())
        {
            value->value = nullptr;
        }
    }
}
