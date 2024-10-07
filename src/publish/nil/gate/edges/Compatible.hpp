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
    struct CompatibilityError
    {
        Error compatibility = Check<concepts::is_compatible<TO, FROM>>();
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
            requires(!concepts::is_compatible<TO, FROM>)
        Compatible(edges::ReadOnly<FROM>* init_edge, errors::CompatibilityError<TO, FROM> = {})
            = delete;

        template <typename FROM>
            requires(concepts::is_compatible<TO, FROM>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Compatible(edges::ReadOnly<FROM>* edge)
            : Compatible(static_cast<detail::edges::Data<FROM>*>(edge)->template adapt<TO>())
        {
        }

        ~Compatible() noexcept = default;

        Compatible(Compatible&&) noexcept = default;
        Compatible& operator=(Compatible&&) noexcept = default;

        Compatible(const Compatible&) = default;
        Compatible& operator=(const Compatible&) = default;

        const TO& value() const
        {
            return value_impl(adapter);
        }

        void attach(INode* node)
        {
            attach_impl(adapter, node);
        }

        bool is_ready() const
        {
            return is_ready_impl(adapter);
        }

    private:
        void* adapter;
        void (*attach_impl)(void*, INode*);
        bool (*is_ready_impl)(const void*);
        const TO& (*value_impl)(const void*);

        template <typename Adapter>
        explicit Compatible(Adapter* init_adapter)
            : adapter(init_adapter)
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
