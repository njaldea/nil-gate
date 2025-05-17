#pragma once

namespace nil::gate
{
    class IPort
    {
    public:
        IPort() = default;
        virtual ~IPort() noexcept = default;

        IPort(IPort&&) noexcept = delete;
        IPort& operator=(IPort&&) noexcept = delete;

        IPort(const IPort&) = delete;
        IPort& operator=(const IPort&) = delete;
    };
}
