#pragma once

#include <nil/xalt/fn_sign.hpp>

#include <nil/xalt/tlist.hpp>

#include <memory>

namespace nil::gate
{
    template <typename CB>
    struct ICallable;

    template <typename R, typename... A>
    struct ICallable<R(A...)>
    {
        ICallable() = default;
        virtual ~ICallable() noexcept = default;

        ICallable(ICallable&&) noexcept = delete;
        ICallable& operator=(ICallable&&) noexcept = delete;

        ICallable(const ICallable&) = delete;
        ICallable& operator=(const ICallable&) = delete;

        virtual R call(A...) = 0;
    };

    template <typename CB>
    std::unique_ptr<ICallable<typename xalt::fn_sign<CB>::free_type>> make_callable(CB&& cb)
    {
        return []<typename... Args>(CB&& c, xalt::tlist<Args...>)
        {
            struct Callable: ICallable<typename xalt::fn_sign<CB>::free_type>
            {
                explicit Callable(CB init_cb)
                    : cb(std::move(init_cb))
                {
                }

                ~Callable() noexcept override = default;

                Callable(Callable&&) noexcept = delete;
                Callable& operator=(Callable&&) noexcept = delete;

                Callable(const Callable&) = delete;
                Callable& operator=(const Callable&) = delete;

                void call(Args... args) override
                {
                    cb(args...);
                }

                CB cb;
            };

            return std::make_unique<Callable>(std::forward<CB>(c));
        }(std::forward<CB>(cb), typename xalt::fn_sign<CB>::arg_types());
    }
}
