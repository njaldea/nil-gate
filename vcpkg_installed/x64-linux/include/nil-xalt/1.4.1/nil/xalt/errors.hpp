#pragma once

namespace nil::xalt
{
    template <typename... T>
    void undefined() = delete;

    template <auto... T>
    void undefined() = delete;
}
