#pragma once

#include <nil/xalt/fn_sign.hpp>

#include <functional>
#include <utility>

namespace nil::gate::nodes
{
    namespace detail
    {
        template <typename T, typename Return, typename Inputs>
        struct ScopedCRTP;

        template <typename T, typename Return, typename... Inputs>
        struct ScopedCRTP<T, Return, xalt::tlist<Inputs...>>
        {
            auto operator()(const Inputs&... args) const
            {
                auto* self = static_cast<const T*>(this);
                if constexpr (std::is_same_v<Return, void>)
                {
                    self->pre();
                    std::invoke(self->node, args...);
                    self->post();
                }
                else
                {
                    self->pre();
                    auto r = std::invoke(self->node, args...);
                    self->post();
                    return r;
                }
            }
        };
    }

    template <typename T, typename Pre, typename Post>
    struct Scoped final
        : detail::ScopedCRTP<
              Scoped<T, Pre, Post>,
              typename xalt::fn_sign<T>::return_type,
              typename xalt::fn_sign<T>::arg_types>
    {
        Scoped(Pre init_pre, T init_node, Post init_post)
            : pre(std::move(init_pre))
            , node{std::move(init_node)}
            , post(std::move(init_post))
        {
        }

        Scoped() = delete;
        ~Scoped() noexcept = default;

        Scoped(Scoped&&) = default;
        Scoped(const Scoped&) = delete;
        Scoped& operator=(Scoped&&) = default;
        Scoped& operator=(const Scoped&) = delete;

        using detail::ScopedCRTP<
            Scoped<T, Pre, Post>,
            typename xalt::fn_sign<T>::return_type,
            typename xalt::fn_sign<T>::arg_types>::operator();

        Pre pre;
        T node;
        Post post;
    };
}
