#include <nil/gate.hpp>

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
    ASSERT_TRUE((std::is_same_v<type::inputs::ports, nil::gate::inputs<>>));

    ASSERT_EQ(type::req_outputs::size, 0);
    ASSERT_TRUE((std::is_same_v<type::req_outputs::ports, nil::gate::req_outputs<>>));

    ASSERT_EQ(type::opt_outputs::size, 0);
    ASSERT_TRUE((std::is_same_v<type::opt_outputs::ports, nil::gate::opt_outputs<>>));

    ASSERT_EQ(type::outputs::size, 0);
    ASSERT_TRUE((std::is_same_v<type::outputs::ports, nil::gate::outputs<>>));

    ASSERT_TRUE(type::is_valid);
    ASSERT_FALSE(type::has_opt);
    ASSERT_FALSE(type::has_core);
}

TEST(gate, traits_input)
{
    { // ok cases
        using test_t = TC<void(
            int,
            const int,
            const int&,
            const std::unique_ptr<int>&,
            const std::shared_ptr<int>&,
            const std::optional<int>&,
            std::shared_ptr<int>,
            std::optional<int>,
            const std::unique_ptr<const int>&,
            const std::shared_ptr<const int>&,
            const std::optional<const int>&,
            std::shared_ptr<const int>,
            std::optional<const int> //
        )>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::inputs::size, 13);
        ASSERT_TRUE( //
            (std::is_same_v<
                type::inputs::ports,
                nil::gate::inputs<
                    int,
                    int,
                    int,
                    std::unique_ptr<int>,
                    std::shared_ptr<int>,
                    std::optional<int>,
                    std::shared_ptr<int>,
                    std::optional<int>,
                    std::unique_ptr<const int>,
                    std::shared_ptr<const int>,
                    std::optional<const int>,
                    std::shared_ptr<const int>,
                    std::optional<const int>>>)
        );

        ASSERT_TRUE(type::is_valid);
        ASSERT_FALSE(type::has_opt);
        ASSERT_FALSE(type::has_core);
    }
    { // invalid cases
        ASSERT_FALSE(SUT<TC<void(int*)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(int&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<int>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<int>&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<int>&&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<const int>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<const int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<const int>&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<const int>&)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(std::unique_ptr<const int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::shared_ptr<const int>&&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(std::optional<const int>&&)>>::is_valid);
    }
}

TEST(gate, traits_req_output)
{
    { // ok cases
        using req_output_t = std::tuple<
            int,
            std::unique_ptr<int>,
            std::shared_ptr<int>,
            std::optional<int>,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using test_t = TC<req_output_t()>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::req_outputs::size, 7);
        ASSERT_EQ(type::outputs::size, 7);

        using expected_t = nil::gate::req_outputs<
            int,
            std::unique_ptr<int>,
            std::shared_ptr<int>,
            std::optional<int>,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        ASSERT_TRUE((std::is_same_v<expected_t, type::req_outputs::ports>));

        ASSERT_TRUE(type::is_valid);
        ASSERT_FALSE(type::has_opt);
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

TEST(gate, traits_opt_output)
{
    { // ok cases
        using opt_output_t = nil::gate::opt_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using test_t = TC<void(opt_output_t)>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::opt_outputs::size, 4);
        ASSERT_EQ(type::outputs::size, 4);

        using expected_t = nil::gate::opt_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        ASSERT_TRUE((std::is_same_v<expected_t, type::opt_outputs::ports>));

        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::has_opt);
        ASSERT_FALSE(type::has_core);
    }
    { // invalid cases
        using namespace nil::gate;
        ASSERT_FALSE(SUT<TC<void(opt_outputs<int*>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<int&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const int>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(opt_outputs<std::unique_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<std::shared_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<std::optional<int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::unique_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::shared_ptr<int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::optional<int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(opt_outputs<std::unique_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<std::shared_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<std::optional<const int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::unique_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::shared_ptr<const int>&>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::optional<const int>&>)>>::is_valid);

        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::unique_ptr<const int>>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::shared_ptr<const int>>)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<const std::optional<const int>>)>>::is_valid);
    }
}

TEST(gate, traits_opt_output_with_core)
{
    { // ok cases
        using opt_output_t = nil::gate::opt_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using test_t = TC<void(const nil::gate::Core&, opt_output_t)>;
        using type = SUT<test_t>;

        ASSERT_EQ(type::opt_outputs::size, 4);
        ASSERT_EQ(type::outputs::size, 4);

        using expected_t = nil::gate::opt_outputs<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        ASSERT_TRUE((std::is_same_v<expected_t, type::opt_outputs::ports>));

        ASSERT_TRUE(type::is_valid);
        ASSERT_TRUE(type::has_opt);
        ASSERT_TRUE(type::has_core);
    }
    { // invalid cases
        using namespace nil::gate;
        ASSERT_FALSE(SUT<TC<void(opt_outputs<int>, nil::gate::Core&)>>::is_valid);
        ASSERT_FALSE(SUT<TC<void(opt_outputs<int>, nil::gate::Core)>>::is_valid);
    }
}

TEST(gate, traits_output)
{
    { // ok cases
        {
            using req_output_t = std::tuple<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using opt_output_t = nil::gate::opt_outputs<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using test_t = TC<req_output_t(opt_output_t)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::ports,
                    nil::gate::outputs<
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>,
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>>>)
            );

            ASSERT_TRUE(type::is_valid);
            ASSERT_TRUE(type::has_opt);
            ASSERT_FALSE(type::has_core);
        }
        {
            using req_output_t = std::tuple<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using opt_output_t = nil::gate::opt_outputs<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using test_t = TC<req_output_t(const opt_output_t&)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::ports,
                    nil::gate::outputs<
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>,
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>>>)
            );

            ASSERT_TRUE(type::is_valid);
            ASSERT_TRUE(type::has_opt);
            ASSERT_FALSE(type::has_core);
        }
        {
            using req_output_t = std::tuple<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using opt_output_t = nil::gate::opt_outputs<
                int,
                std::unique_ptr<const int>,
                std::shared_ptr<const int>,
                std::optional<const int>>;
            using test_t = TC<req_output_t(const nil::gate::Core&, const opt_output_t&)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::ports,
                    nil::gate::outputs<
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>,
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>>>)
            );

            ASSERT_TRUE(type::is_valid);
            ASSERT_TRUE(type::has_opt);
            ASSERT_TRUE(type::has_core);
        }
    }
    { // invalid cases
        using req_output_t = std::tuple<
            int,
            std::unique_ptr<const int>,
            std::shared_ptr<const int>,
            std::optional<const int>>;
        using opt_output_t = nil::gate::opt_outputs<
            char,
            std::unique_ptr<const char>,
            std::shared_ptr<const char>,
            std::optional<const char>>;

        {
            using test_t = TC<req_output_t(opt_output_t&)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::ports,
                    nil::gate::outputs<
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>,
                        char,
                        std::unique_ptr<const char>,
                        std::shared_ptr<const char>,
                        std::optional<const char>>>)
            );

            ASSERT_FALSE(type::is_valid);
            ASSERT_TRUE(type::has_opt);
            ASSERT_FALSE(type::has_core);
        }
        {
            using test_t = TC<req_output_t(nil::gate::Core&, opt_output_t)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::ports,
                    nil::gate::outputs<
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>,
                        char,
                        std::unique_ptr<const char>,
                        std::shared_ptr<const char>,
                        std::optional<const char>>>)
            );

            ASSERT_FALSE(type::is_valid);
            ASSERT_TRUE(type::has_opt);
            ASSERT_TRUE(type::has_core);
        }
        {
            using test_t = TC<req_output_t(nil::gate::Core, opt_output_t)>;
            using type = SUT<test_t>;

            ASSERT_EQ(type::outputs::size, 8);
            ASSERT_TRUE( //
                (std::is_same_v<
                    type::outputs::ports,
                    nil::gate::outputs<
                        int,
                        std::unique_ptr<const int>,
                        std::shared_ptr<const int>,
                        std::optional<const int>,
                        char,
                        std::unique_ptr<const char>,
                        std::shared_ptr<const char>,
                        std::optional<const char>>>)
            );

            ASSERT_FALSE(type::is_valid);
            ASSERT_TRUE(type::has_opt);
            ASSERT_TRUE(type::has_core);
        }
    }
}
