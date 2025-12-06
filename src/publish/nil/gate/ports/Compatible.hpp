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
            , ptr_attach_out(&impl_attach_out<detail::Port<TO>>)
            , ptr_detach_out(&impl_detach_out<detail::Port<TO>>)
            , ptr_is_ready(&impl_is_ready<detail::Port<TO>>)
            , ptr_score(&impl_score<detail::Port<TO>>)
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
            // should not be possible based on how it is called
            return ptr_value(context);
        }

        void attach_out(INode* node)
        {
            if (nullptr == context)
            {
                return;
            }
            ptr_attach_out(context, node);
        }

        void detach_out(INode* node)
        {
            if (nullptr == context)
            {
                return;
            }
            ptr_detach_out(context, node);
        }

        bool detach(IPort* port)
        {
            if (port == context)
            {
                context = nullptr;
                return true;
            }
            return false;
        }

        int score() const noexcept
        {
            if (nullptr == context)
            {
                return 0;
            }
            return ptr_score(context);
        }

        bool is_ready() const
        {
            if (nullptr == context)
            {
                return false;
            }
            return ptr_is_ready(context);
        }

    private:
        void* context = nullptr;
        void (*ptr_attach_out)(void*, INode*) = nullptr;
        void (*ptr_detach_out)(void*, INode*) = nullptr;
        bool (*ptr_is_ready)(const void*) = nullptr;
        int (*ptr_score)(const void*) = nullptr;
        const TO& (*ptr_value)(const void*) = nullptr;

        template <typename T>
        static void impl_attach_out(void* port, INode* node)
        {
            static_cast<T*>(port)->attach_out(node);
        }

        template <typename T>
        static void impl_detach_out(void* port, INode* node)
        {
            static_cast<T*>(port)->detach_out(node);
        }

        template <typename T>
        static bool impl_is_ready(const void* port)
        {
            return static_cast<const T*>(port)->is_ready();
        }

        template <typename T>
        static int impl_score(const void* port)
        {
            return static_cast<const T*>(port)->score();
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
            , ptr_attach_out(&impl_attach_out<Adapter>)
            , ptr_is_ready(&impl_is_ready<Adapter>)
            , ptr_value(&impl_value<Adapter>)
        {
        }
    };
}
