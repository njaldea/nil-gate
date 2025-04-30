#pragma once

#include "ICallable.hpp"

#include <mutex>
#include <utility>
#include <vector>

namespace nil::gate
{
    class Diffs final
    {
    public:
        Diffs() = default;
        ~Diffs() noexcept = default;

        Diffs(Diffs&&) noexcept = delete;
        Diffs& operator=(Diffs&&) noexcept = delete;

        Diffs(const Diffs&) = delete;
        Diffs& operator=(const Diffs&) = delete;

        void push(std::unique_ptr<ICallable<void()>> cb)
        {
            std::lock_guard g(mutex);
            diffs.push_back(std::move(cb));
        }

        std::unique_ptr<ICallable<void()>> flush()
        {
            std::lock_guard g(mutex);
            return make_callable(
                [dd = std::exchange(diffs, {})]()
                {
                    for (const auto& d : dd)
                    {
                        if (d)
                        {
                            d->call();
                        }
                    }
                }
            );
        }

    private:
        std::vector<std::unique_ptr<ICallable<void()>>> diffs;
        std::mutex mutex;
    };
}
