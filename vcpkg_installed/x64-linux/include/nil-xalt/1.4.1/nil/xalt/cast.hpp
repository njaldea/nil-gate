#pragma once

#include <type_traits>

namespace nil::xalt
{
    template <typename T>
    struct explicit_cast final
    {
        explicit explicit_cast(std::remove_reference_t<T>* init_ptr)
            : ptr(init_ptr)
        {
        }

        explicit_cast(explicit_cast&&) = delete;
        explicit_cast(const explicit_cast&) = default;
        explicit_cast& operator=(explicit_cast&&) = delete;
        explicit_cast& operator=(const explicit_cast&) = default;

        ~explicit_cast() noexcept = default;

        T cast() const
        {
            return static_cast<T>(*ptr);
        }

    private:
        std::remove_reference_t<T>* ptr;
    };

    template <typename T>
    struct implicit_cast final
    {
    public:
        explicit implicit_cast(std::remove_reference_t<T>* init_ptr)
            : ptr(init_ptr)
        {
        }

        implicit_cast(implicit_cast&&) = delete;
        implicit_cast(const implicit_cast&) = default;
        implicit_cast& operator=(implicit_cast&&) = delete;
        implicit_cast& operator=(const implicit_cast&) = default;

        ~implicit_cast() noexcept = default;

        template <typename U>
        operator U() const = delete;

        operator T() const // NOLINT(hicpp-explicit-conversions))
        {
            return static_cast<T>(*ptr);
        }

    private:
        std::remove_reference_t<T>* ptr;
    };
}
