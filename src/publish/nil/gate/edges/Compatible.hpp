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

    template <typename TO, typename FROM>
    concept is_ref_return = std::is_reference_v<decltype( //
        traits::compatibility<TO, FROM>::convert(std::declval<FROM>())
    )>;
}

namespace nil::gate::errors
{
    template <typename TO, typename FROM>
    struct CompatibilityError
    {
        Error compatibility = Check<concepts::is_compatible<TO, FROM>>();
    };
}

template <typename T>
void printer_nil();

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
            : adapter(static_cast<detail::edges::Data<FROM>*>(edge)->template adapt<TO>())
            , attach_impl( //
                  +[](void* p, INode* node)
                  {
                      using DATA = detail::edges::Data<FROM>;
                      using type = decltype(std::declval<DATA>().template adapt<TO>());
                      static_cast<type>(p)->attach(node);
                  }
              )
            , is_ready_impl( //
                  +[](void* p)
                  {
                      using DATA = detail::edges::Data<FROM>;
                      using type = decltype(std::declval<DATA>().template adapt<TO>());
                      return static_cast<type>(p)->is_ready();
                  }
              )
            , value_impl( //
                  +[](void* p) -> const TO&
                  {
                      using DATA = detail::edges::Data<FROM>;
                      using type = decltype(std::declval<DATA>().template adapt<TO>());
                      return static_cast<type>(p)->value();
                  }
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
        void* adapter = nullptr;
        void (*attach_impl)(void*, INode*);
        bool (*is_ready_impl)(void*);
        const TO& (*value_impl)(void*);
    };
}
