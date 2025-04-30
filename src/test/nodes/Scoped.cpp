#include <nil/gate.hpp>

#include <nil/gate/nodes/Scoped.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(nodes, scoped_blank)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    core.node(nil::gate::nodes::Scoped(
        [&]() { mocked_fn.Call("PRE"); },
        [&]() { mocked_fn.Call("NODE"); },
        [&]() { mocked_fn.Call("POST"); }
    ));

    EXPECT_CALL(mocked_fn, Call("PRE")) //
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(mocked_fn, Call("POST")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();
}

TEST(nodes, scoped_with_sync_out)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    const auto [e] = core.node(nil::gate::nodes::Scoped(
        [&]() { mocked_fn.Call("PRE"); },
        [&]()
        {
            mocked_fn.Call("NODE");
            return 100;
        },
        [&]() { mocked_fn.Call("POST"); }
    ));

    EXPECT_CALL(mocked_fn, Call("PRE")) //
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(mocked_fn, Call("POST")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();

    ASSERT_EQ(e->value(), 100);
}
