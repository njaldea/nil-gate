#include <nil/gate.hpp>

#include <nil/gate/bias/nil.hpp>

#include <gtest/gtest.h>

template <typename U>
struct TC;

template <typename... A>
struct TC<void(A...)>: TC<std::tuple<>(A...)>
{
};

template <typename... R, typename... A>
struct TC<std::tuple<R...>(A...)>
{
    std::tuple<R...> operator()(A... /*unused*/) const
    {
        return std::make_tuple(R()...);
    }
};

template <typename T>
using SUT = nil::gate::detail::traits::node<T>;

TEST(gate, traits_empty)
{
    using type = SUT<TC<void()>>;

    ASSERT_EQ(type::inputs::size, 0);
    ASSERT_TRUE((std::is_same_v<type::inputs::edges, nil::gate::inputs<>>));

    ASSERT_EQ(type::sync_outputs::size, 0);
    ASSERT_TRUE((std::is_same_v<type::sync_outputs::edges, nil::gate::sync_outputs<>>));

    ASSERT_EQ(type::async_outputs::size, 0);
    ASSERT_TRUE((std::is_same_v<type::async_outputs::edges, nil::gate::async_outputs<>>));

    ASSERT_EQ(type::outputs::size, 0);
    ASSERT_TRUE((std::is_same_v<
                 type::outputs::edges,
                 nil::gate::outputs<nil::gate::sync_outputs<>, nil::gate::async_outputs<>>>));

    ASSERT_TRUE(type::is_valid);
    ASSERT_FALSE(type::has_async);
    ASSERT_FALSE(type::has_core);
}

TEST(gate, traits_input)
{
    { // ok cases
        using test_t = TC<void(
            int,
            const int,
            const int&,
            const std::unique_ptr<const int>&,
            const std::shared_ptr<const int>&,
            const std::optional<const int>&,
            std::shared_ptr<const int>,
            std::optional<const int> //
        )>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::inputs::size, 8);
        ASSERT_TRUE( //
            (std::is_same_v<
                type::inputs::edges,
                nil::gate::inputs<
                    int,
                    int,
                    int,
                    std::unique_ptr<const int>,
                    std::shared_ptr<const int>,
                    std::optional<const int>,
                    std::shared_ptr<const int>,
                    std::optional<const int>>>)
        );

        ASSERT_TRUE(type::is_valid);
        ASSERT_FALSE(type::has_async);
        ASSERT_FALSE(type::has_core);
    }
    { // invalid cases
        ASSERT_FALSE(SUT<TC<void(int*)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(int&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<int>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<int>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<int>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<int>&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(const std::unique_ptr<int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(const std::shared_ptr<int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(const std::optional<int>&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(const std::unique_ptr<int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(const std::shared_ptr<int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(const std::optional<int>&&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<const int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<const int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<const int>&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<const int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<const int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<const int>&&)>>::is_valid);
    }
}

TEST(gate, traits_sync_output)
{
    { // ok cases
        using sync_output_t = std::tuple<
            int,
            std::unique_ptr<int>,
            std::shared_ptr<int>,
            std::optional<int>,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using test_t = TC<sync_output_t()>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::sync_outputs::size, 7);
        ASSERT_EQ(type::outputs::size, 7);

        using expected_t = nil::gate::sync_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;

        ASSERT_TRUE((std::is_same_v<type::sync_outputs::edges, expected_t>));

        ASSERT_TRUE(type::is_valid);
        ASSERT_FALSE(type::has_async);
        ASSERT_FALSE(type::has_core);
    }
    { // invalid cases
        ASSERT_FALSE(SUT<TC<std::tuple<int*>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<int&>()>>::is_valid);

        ASSERT_FALSE(SUT<TC<std::tuple<std::unique_ptr<int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<std::shared_ptr<int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<std::optional<int>&>()>>::is_valid);

        ASSERT_FALSE(SUT<TC<std::tuple<const std::unique_ptr<int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<const std::shared_ptr<int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<const std::optional<int>&>()>>::is_valid);

        ASSERT_FALSE(SUT<TC<std::tuple<std::unique_ptr<const int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<std::shared_ptr<const int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<std::optional<const int>&>()>>::is_valid);

        ASSERT_FALSE(SUT<TC<std::tuple<const std::unique_ptr<const int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<const std::shared_ptr<const int>&>()>>::is_valid);
        ASSERT_FALSE(SUT<TC<std::tuple<const std::optional<const int>&>()>>::is_valid);
    }
}

TEST(gate, traits_async_output)
{
    { // ok cases
        using async_output_t = nil::gate::async_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using test_t = TC<void(async_output_t)>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::async_outputs::size, 4);
        ASSERT_EQ(type::outputs::size, 4);

        using expected_t = nil::gate::async_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        ASSERT_TRUE((std::is_same_v<type::async_outputs::edges, expected_t>));

        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::has_async);
        ASSERT_FALSE(type::has_core);
    }
    { // invalid cases
        using namespace nil::gate;
        ASSERT_FALSE(SUT<TC<void(async_outputs<int*>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<int&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const int>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(async_outputs<std::unique_ptr<int>>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<std::shared_ptr<int>>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<std::optional<int>>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(async_outputs<std::unique_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<std::shared_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<std::optional<int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::unique_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::shared_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::optional<int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(async_outputs<std::unique_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<std::shared_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<std::optional<const int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::unique_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::shared_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::optional<const int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::unique_ptr<const int>>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::shared_ptr<const int>>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(async_outputs<const std::optional<const int>>)>>::is_valid);
    }
}

TEST(gate, traits_async_output_with_core)
{
    { // ok cases
        using async_output_t = nil::gate::async_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using test_t = TC<void(const nil::gate::Core&, async_output_t)>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::async_outputs::size, 4);
        ASSERT_EQ(type::outputs::size, 4);

        using expected_t = nil::gate::async_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        ASSERT_TRUE((std::is_same_v<type::async_outputs::edges, expected_t>));

        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::has_async);
        ASSERT_TRUE(type::has_core);
    }
    { // invalid cases
        using namespace nil::gate;
        ASSERT_FALSE(SUT<TC<void(nil::gate::Core&, async_outputs<int>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(nil::gate::Core, async_outputs<int>)>>::is_valid);
    }
}

TEST(gate, traits_output)
{
    { // ok cases
        {
            using sync_output_t = std::tuple<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using async_output_t = nil::gate::async_outputs<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using test_t = TC<sync_output_t(async_output_t)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::edges,
                    nil::gate::outputs<
                        nil::gate::sync_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>,
                        nil::gate::async_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>>>)
            );

            ASSERT_TRUE(type::is_valid);
            ASSERT_TRUE(type::has_async);
            ASSERT_FALSE(type::has_core);
        }
        {
            using sync_output_t = std::tuple<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using async_output_t = nil::gate::async_outputs<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using test_t = TC<sync_output_t(const async_output_t&)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::edges,
                    nil::gate::outputs<
                        nil::gate::sync_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>,
                        nil::gate::async_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>>>)
            );

            ASSERT_TRUE(type::is_valid);
            ASSERT_TRUE(type::has_async);
            ASSERT_FALSE(type::has_core);
        }
        {
            using sync_output_t = std::tuple<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using async_output_t = nil::gate::async_outputs<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using test_t = TC<sync_output_t(const nil::gate::Core&, const async_output_t&)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::edges,
                    nil::gate::outputs<
                        nil::gate::sync_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>,
                        nil::gate::async_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>>>)
            );

            ASSERT_TRUE(type::is_valid);
            ASSERT_TRUE(type::has_async);
            ASSERT_TRUE(type::has_core);
        }
    }
    { // invalid cases
        using sync_output_t = std::tuple<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using async_output_t = nil::gate::async_outputs<
            char,
            std::unique_ptr<const char>,
            std::shared_ptr<const char>,
            std::optional<const char>>;

        {
            using test_t = TC<sync_output_t(async_output_t&)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::edges,
                    nil::gate::outputs<
                        nil::gate::sync_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>,
                        nil::gate::async_outputs<
                            char,
                            std::unique_ptr<const char>,
                            std::shared_ptr<const char>,
                            std::optional<const char>>>>)
            );

            ASSERT_FALSE(type::is_valid);
            ASSERT_TRUE(type::has_async);
            ASSERT_FALSE(type::has_core);
        }
        {
            using test_t = TC<sync_output_t(nil::gate::Core&, async_output_t)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::edges,
                    nil::gate::outputs<
                        nil::gate::sync_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>,
                        nil::gate::async_outputs<
                            char,
                            std::unique_ptr<const char>,
                            std::shared_ptr<const char>,
                            std::optional<const char>>>>)
            );

            ASSERT_FALSE(type::is_valid);
            ASSERT_TRUE(type::has_async);
            ASSERT_TRUE(type::has_core);
        }
        {
            using test_t = TC<sync_output_t(nil::gate::Core, async_output_t)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::edges,
                    nil::gate::outputs<
                        nil::gate::sync_outputs<
                            int,
                            std::unique_ptr<const int>,
                            std::shared_ptr<const int>,
                            std::optional<const int>>,
                        nil::gate::async_outputs<
                            char,
                            std::unique_ptr<const char>,
                            std::shared_ptr<const char>,
                            std::optional<const char>>>>)
            );

            ASSERT_FALSE(type::is_valid);
            ASSERT_TRUE(type::has_async);
            ASSERT_TRUE(type::has_core);
        }
    }
}
