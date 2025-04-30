#pragma once

#include "../detail/DataEdge.hpp"
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

namespace nil::gate::edges
{
    template <typename TO>
    class Compatible final
    {
    public:
        Compatible() = delete;

        template <typename FROM>
            requires(!concepts::is_compatible<TO, FROM> && !std::is_same_v<TO, FROM>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(edges::ReadOnly<FROM>* /*edge*/)
            : Compatible(errors::Compatibility<TO, FROM>())
        {
        }

        template <typename FROM>
            requires(concepts::is_compatible<TO, FROM> && !std::is_same_v<TO, FROM>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(edges::ReadOnly<FROM>* edge)
            : Compatible(static_cast<detail::edges::Data<FROM>*>(edge)->template adapt<TO>())
        {
        }

        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(edges::ReadOnly<TO>* edge)
            : context(edge)
            , attach_impl( //
                  +[](void* p, INode* node)
                  { static_cast<detail::edges::Data<TO>*>(p)->attach(node); }
              )
            , is_ready_impl( //
                  +[](const void* p)
                  { return static_cast<const detail::edges::Data<TO>*>(p)->is_ready(); }
              )
            , value_impl( //
                  +[](const void* p) -> const TO&
                  { return static_cast<const detail::edges::Data<TO>*>(p)->value(); }
              )
        {
        }

        ~Compatible() noexcept = default;

        Compatible(Compatible&&) noexcept = default;
        Compatible& operator=(Compatible&&) noexcept = default;

        Compatible(const Compatible&) = default;
        Compatible& operator=(const Compatible&) = default;

        const TO& value() const
        {
            return value_impl(context);
        }

        void attach(INode* node)
        {
            attach_impl(context, node);
        }

        bool is_ready() const
        {
            return is_ready_impl(context);
        }

    private:
        void* context = nullptr;
        void (*attach_impl)(void*, INode*) = nullptr;
        bool (*is_ready_impl)(const void*) = nullptr;
        const TO& (*value_impl)(const void*) = nullptr;

        template <typename Adapter>
            requires(!std::is_base_of_v<IEdge, Adapter>)
        explicit Compatible(Adapter* adapter)
            : context(adapter)
            , attach_impl( //
                  +[](void* p, INode* node) { static_cast<Adapter*>(p)->attach(node); }
              )
            , is_ready_impl( //
                  +[](const void* p) { return static_cast<const Adapter*>(p)->is_ready(); }
              )
            , value_impl( //
                  +[](const void* p) -> const TO&
                  { return static_cast<const Adapter*>(p)->value(); }
              )
        {
        }
    };
}
