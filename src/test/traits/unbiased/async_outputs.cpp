#include <nil/gate.hpp>

#include <gtest/gtest.h>

template <typename T>
using SUT = nil::gate::detail::traits::node_async_outputs<T>;

template <typename... T>
using async_outputs = nil::gate::async_outputs<T...>;

template <typename... T>
using types = nil::gate::detail::traits::types<T...>;

TEST(gate_node_async_outputs, traits)
{
    {
        using type = SUT<types<>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 0);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<>>));
    }
    {
        using type = SUT<types<int>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<int>>));
    }
    {
        ASSERT_FALSE(SUT<types<int*>>::is_valid);
        ASSERT_FALSE(SUT<types<int&>>::is_valid);
        ASSERT_FALSE(SUT<types<int&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const int*>>::is_valid);
        ASSERT_FALSE(SUT<types<const int>>::is_valid);
        ASSERT_FALSE(SUT<types<const int&>>::is_valid);
        ASSERT_FALSE(SUT<types<const int&&>>::is_valid);
    }
}

TEST(gate_node_async_outputs, traits_unique_ptr)
{
    {
        ASSERT_FALSE(SUT<types<std::unique_ptr<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::unique_ptr<int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::unique_ptr<int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::unique_ptr<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::unique_ptr<int>&&>>::is_valid);

        ASSERT_FALSE(SUT<types<std::unique_ptr<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::unique_ptr<const int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::unique_ptr<const int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::unique_ptr<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::unique_ptr<const int>&&>>::is_valid);
    }
    {
        using type = SUT<types<std::unique_ptr<int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::unique_ptr<int>>>));
    }
    {
        using type = SUT<types<std::unique_ptr<const int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::unique_ptr<const int>>>));
    }
}

TEST(gate_node_async_outputs, traits_shared_ptr)
{
    {
        ASSERT_FALSE(SUT<types<std::shared_ptr<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::shared_ptr<int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::shared_ptr<int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::shared_ptr<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::shared_ptr<int>&&>>::is_valid);

        ASSERT_FALSE(SUT<types<std::shared_ptr<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::shared_ptr<const int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::shared_ptr<const int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::shared_ptr<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::shared_ptr<const int>&&>>::is_valid);
    }
    {
        using type = SUT<types<std::shared_ptr<int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::shared_ptr<int>>>));
    }
    {
        using type = SUT<types<std::shared_ptr<const int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::shared_ptr<const int>>>));
    }
}

TEST(gate_node_async_outputs, traits_optional)
{
    {
        ASSERT_FALSE(SUT<types<std::optional<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::optional<int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::optional<int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::optional<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::optional<int>&&>>::is_valid);

        ASSERT_FALSE(SUT<types<std::optional<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::optional<const int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::optional<const int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::optional<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::optional<const int>&&>>::is_valid);
    }
    {
        using type = SUT<types<std::optional<int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::optional<int>>>));
    }
    {
        using type = SUT<types<std::optional<const int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::optional<const int>>>));
    }
}

TEST(gate_node_async_outputs, traits_ref_wrapper)
{
    {
        ASSERT_FALSE(SUT<types<std::reference_wrapper<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::reference_wrapper<int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::reference_wrapper<int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::reference_wrapper<int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::reference_wrapper<int>&&>>::is_valid);

        ASSERT_FALSE(SUT<types<std::reference_wrapper<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<std::reference_wrapper<const int>&&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::reference_wrapper<const int>>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::reference_wrapper<const int>&>>::is_valid);
        ASSERT_FALSE(SUT<types<const std::reference_wrapper<const int>&&>>::is_valid);
    }
    {
        using type = SUT<types<std::reference_wrapper<int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::reference_wrapper<int>>>));
    }
    {
        using type = SUT<types<std::reference_wrapper<const int>>>;
        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::size == 1);
        ASSERT_TRUE((std::is_same_v<type::edges, async_outputs<std::reference_wrapper<const int>>>)
        );
    }
}
