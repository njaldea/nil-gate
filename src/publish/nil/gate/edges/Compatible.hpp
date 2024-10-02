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
    template <typename T, typename U>
    concept is_compatible = requires(T t, U u) {
        { traits::compatibility<T, U>::convert(u) };
    };

    template <typename T, typename U>
    concept is_ref_return = std::is_reference_v<                          //
        decltype(traits::compatibility<T, U>::convert(std::declval<U>())) //
        >;
}

namespace nil::gate::errors
{
    template <typename T, typename U>
    struct CompatibilityError
    {
        Error compatibility = Check<concepts::is_compatible<T, U>>();
    };
}

namespace nil::gate::edges
{
    template <typename T>
    class Compatible final
    {
    public:
        Compatible() = delete;

        template <typename U>
            requires(!concepts::is_compatible<T, U>)
        Compatible(edges::ReadOnly<U>* init_edge, errors::CompatibilityError<T, U> = {}) = delete;

        template <typename U>
            requires(concepts::is_compatible<T, U>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        constexpr Compatible(edges::ReadOnly<U>* edge)
            : adapter(static_cast<detail::edges::Data<U>*>(edge)->template adapt<T>())
            , attach_impl( //
                  +[](void* p, INode* node)
                  {
                      using type
                          = decltype(std::declval<detail::edges::Data<U>>().template adapt<T>());
                      static_cast<type>(p)->attach(node);
                  }
              )
            , is_ready_impl( //
                  +[](void* p)
                  {
                      using type
                          = decltype(std::declval<detail::edges::Data<U>>().template adapt<T>());
                      return static_cast<type>(p)->is_ready();
                  }
              )
            , value_impl( //
                  +[](void* p) -> const T&
                  {
                      using type
                          = decltype(std::declval<detail::edges::Data<U>>().template adapt<T>());
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

        const T& value() const
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
        const T& (*value_impl)(void*);
    };
}
