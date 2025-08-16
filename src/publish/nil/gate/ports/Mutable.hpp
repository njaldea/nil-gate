#pragma once

#include "ReadOnly.hpp"

namespace nil::gate::ports
{
    /**
     * @brief Mutable/Readable Port type returned by Core::port.
     *
     * @tparam T
     */
    template <typename T>
    class Mutable: public ReadOnly<T>
    {
    public:
        Mutable() = default;
        ~Mutable() noexcept override = default;

        Mutable(Mutable&&) noexcept = delete;
        Mutable& operator=(Mutable&&) noexcept = delete;

        Mutable(const Mutable&) = delete;
        Mutable& operator=(const Mutable&) = delete;

        /**
         * @brief Set the value object. Effect is deferred. To be applied on next core.commit()
         */
        virtual void set_value(T new_data) = 0;

        /**
         * @brief Unset the value. Effect is deferred. To be applied on next core.commit()
         */
        virtual void unset_value() = 0;
    };
}
