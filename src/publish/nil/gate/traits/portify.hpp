// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace nil::gate::traits
{
    template <typename T>
    struct portify
    {
        using type = T;
    };

    template <typename T>
    using portify_t = typename portify<T>::type;
}
