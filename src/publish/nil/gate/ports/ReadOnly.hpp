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

        /**
         * @brief Get the value held by the port.
         *  Ensure the following:
         *   - check `has_value` before calling this method.
         *   - called when the runner is not running, or
         *   - called inside core.post, core.apply, or
         *   - called inside a node
         */
        [[nodiscard]] virtual const T& value() const noexcept = 0;

        /**
         * @brief Check if port is initialized.
         *  Ensure the following:
         *   - called when the runner is not running, or
         *   - called inside core.post, core.apply, or
         *   - called inside a node
         */
        [[nodiscard]] virtual bool has_value() const noexcept = 0;
    };
}
