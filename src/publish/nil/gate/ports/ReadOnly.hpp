#pragma once

#include "../IPort.hpp"

namespace nil::gate::ports
{
    /**
     * @brief Readable Port type returned by Core::node.
     *
     * @tparam T
     */
    template <typename T>
    class ReadOnly: public IPort
    {
    public:
        ReadOnly() = default;
        ~ReadOnly() noexcept override = default;

        ReadOnly(ReadOnly&&) noexcept = delete;
        ReadOnly& operator=(ReadOnly&&) noexcept = delete;

        ReadOnly(const ReadOnly&) = delete;
        ReadOnly& operator=(const ReadOnly&) = delete;

        // For ports created for a Node, make sure to call core.run() before accessing value.
        // For ports created on its own, it should always have a value due to Core's api.
        virtual const T& value() const = 0;
    };
}
