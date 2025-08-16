#pragma once

#include "../detail/Port.hpp"
#include "../errors.hpp"
#include "../traits/compatibility.hpp"

namespace nil::gate
{
    class INode;
}

namespace nil::gate::concepts
{
    template <typename TO, typename FROM>
    concept is_compatible = requires(TO to, FROM from) {
        { traits::compatibility<TO, FROM>::convert(from) };
    };
}

namespace nil::gate::errors
{
    template <typename TO, typename FROM>
    struct Compatibility
    {
        Error compatibility = Check<concepts::is_compatible<TO, FROM>>("Not Compatible");
    };
}

namespace nil::gate::ports
{
    template <typename TO>
    class Compatible final
    {
    public:
        Compatible() = delete;

        template <typename FROM>
            requires(!concepts::is_compatible<TO, FROM> && !std::is_same_v<TO, FROM>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(ports::ReadOnly<FROM>* /*port*/)
            : Compatible(errors::Compatibility<TO, FROM>())
        {
        }

        template <typename FROM>
            requires(concepts::is_compatible<TO, FROM> && !std::is_same_v<TO, FROM>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(ports::ReadOnly<FROM>* port)
            : Compatible(static_cast<detail::Port<FROM>*>(port)->template adapt<TO>())
        {
        }

        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(ports::ReadOnly<TO>* port)
            : context(port)
            , ptr_attach(&impl_attach<detail::Port<TO>>)
            , ptr_is_ready(&impl_is_ready<detail::Port<TO>>)
            , ptr_value(&impl_value<detail::Port<TO>>)
        {
        }

        ~Compatible() noexcept = default;

        Compatible(Compatible&&) noexcept = default;
        Compatible& operator=(Compatible&&) noexcept = default;

        Compatible(const Compatible&) = default;
        Compatible& operator=(const Compatible&) = default;

        const TO& value() const
        {
            return ptr_value(context);
        }

        void attach(INode* node)
        {
            ptr_attach(context, node);
        }

        bool is_ready() const
        {
            return ptr_is_ready(context);
        }

    private:
        void* context = nullptr;
        void (*ptr_attach)(void*, INode*) = nullptr;
        bool (*ptr_is_ready)(const void*) = nullptr;
        const TO& (*ptr_value)(const void*) = nullptr;

        template <typename T>
        static void impl_attach(void* port, INode* node)
        {
            static_cast<T*>(port)->attach(node);
        }

        template <typename T>
        static bool impl_is_ready(const void* port)
        {
            return static_cast<const T*>(port)->is_ready();
        }

        template <typename T>
        static const TO& impl_value(const void* port)
        {
            return static_cast<const T*>(port)->value();
        }

        template <typename Adapter>
            requires(!std::is_base_of_v<IPort, Adapter>)
        explicit Compatible(Adapter* adapter)
            : context(adapter)
            , ptr_attach(&impl_attach<Adapter>)
            , ptr_is_ready(&impl_is_ready<Adapter>)
            , ptr_value(&impl_value<Adapter>)
        {
        }
    };
}
