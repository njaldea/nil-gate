// Copyright (c) 2026, Neil Aldea <njaldea@gmail.com>
// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace nil::gate::errors
{
    struct Error
    {
    };

    template <bool T>
    struct Check
    {
        const char* message;
        // NOLINTNEXTLINE
        operator Error()
            requires(T);
    };

}
