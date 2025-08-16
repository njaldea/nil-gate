#pragma once

#include "ports/Compatible.hpp"
#include "ports/Mutable.hpp"
#include "ports/ReadOnly.hpp"

#include <tuple>

namespace nil::gate
{
    template <typename... T>
    using inputs = std::tuple<ports::Compatible<T>...>;
    template <typename... T>
    using req_outputs = std::tuple<ports::ReadOnly<T>*...>;
    template <typename... T>
    using opt_outputs = std::tuple<ports::Mutable<T>*...>;
    template <typename... T>
    using outputs = std::tuple<ports::ReadOnly<T>*...>;
}
