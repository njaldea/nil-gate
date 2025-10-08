#pragma once

#include <nil/gate.hpp>

namespace nil::gate::c
{
    struct PortType final
    {
        void* value = nullptr;
        int (*eq)(const void*, const void*) = nullptr;
        void (*destroy)(void*) = nullptr;

        explicit PortType(
            void* init_value,
            int (*init_eq)(const void*, const void*),
            void (*init_destroy)(void*)
        );

        PortType(const PortType& o) = delete;
        PortType& operator=(const PortType& o) = delete;
        PortType(PortType&& o) noexcept;
        PortType& operator=(PortType&& o) noexcept;
        ~PortType();
    };
}

namespace nil::gate::traits::port
{
    template <>
    bool is_eq(const c::PortType& current_value, const c::PortType& new_value);

    template <>
    bool has_value(const c::PortType& value);

    template <>
    void unset(std::optional<c::PortType>& value);
}
