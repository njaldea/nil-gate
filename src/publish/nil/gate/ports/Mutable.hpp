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
         * @brief Set the value object.
         */
        virtual void set_value(T new_data) = 0;

        /**
         * @brief Unset the value.
         */
        virtual void unset_value() = 0;

        virtual int score() const noexcept = 0;
    };
}
