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
