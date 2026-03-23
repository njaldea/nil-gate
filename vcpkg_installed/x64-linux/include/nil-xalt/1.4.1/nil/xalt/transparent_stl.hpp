#pragma once

#include <unordered_map>

#include <string>
#include <string_view>

namespace nil::xalt
{
    struct transparent_umap_hash final
    {
        using is_transparent = void;

        std::size_t operator()(std::string_view s) const noexcept
        {
            return std::hash<std::string_view>()(s);
        }
    };

    struct transparent_umap_equal final
    {
        using is_transparent = void;

        bool operator()(std::string_view l, std::string_view r) const
        {
            return l == r;
        }
    };

    // std::string_view only works for .find
    template <typename T>
    using transparent_umap
        = std::unordered_map<std::string, T, transparent_umap_hash, transparent_umap_equal>;
}
