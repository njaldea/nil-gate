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
         *  Ensure that this is called inside the
         *  runner's thread or that the runner is
         *  not running. Ensure to check `has_value`
         *  before calling this method.
         */
        [[nodiscard]] virtual const T& value() const noexcept = 0;

        /**
         * @brief Check if port is initialized.
         *  Ensure that this is called inside the
         *  runner's thread or that the runner is
         *  not running.
         */
        [[nodiscard]] virtual bool has_value() const noexcept = 0;
    };
}
