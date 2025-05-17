#include <nil/gate.hpp>

#include <nil/gate/nodes/Asynced.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(nodes, asynced_blank)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    core.node(nil::gate::nodes::Asynced([&]() { mocked_fn.Call("NODE"); }));

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();
}

TEST(nodes, asynced_input)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    auto* e = core.port(std::string("value"));

    core.node(
        nil::gate::nodes::Asynced(
            [&](const std::string& v)
            {
                mocked_fn.Call("NODE");
                mocked_fn.Call(v);
            }
        ),
        e
    );

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(mocked_fn, Call("value")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();
}

TEST(nodes, asynced_sync_output)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    const auto [e] = core.node(nil::gate::nodes::Asynced(
        [&]()
        {
            mocked_fn.Call("NODE");
            return std::string("value");
        }
    ));

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();

    // WILL SEG FAULT SINCE IT SHOULD NOT HAVE BEEN POPULATED YET
    // ASSERT_EQ(e->value(), "value");

    core.commit();

    ASSERT_EQ(e->value(), "value");
}

TEST(nodes, asynced_async_output)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    const auto [e] = core.node(nil::gate::nodes::Asynced(
        [&](nil::gate::async_outputs<std::string> s)
        {
            mocked_fn.Call("NODE");
            get<0>(s)->set_value("value");
        }
    ));

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();

    // WILL SEG FAULT SINCE IT SHOULD NOT HAVE BEEN POPULATED YET
    // ASSERT_EQ(e->value(), "value");

    core.commit();

    ASSERT_EQ(e->value(), "value");
}

TEST(nodes, asynced_outputs)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    const auto [se, ae] = core.node(nil::gate::nodes::Asynced(
        [&](nil::gate::async_outputs<std::string> s)
        {
            mocked_fn.Call("NODE");
            get<0>(s)->set_value("value");
            return 100;
        }
    ));

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();

    // WILL SEG FAULT SINCE IT SHOULD NOT HAVE BEEN POPULATED YET
    // ASSERT_EQ(e->value(), "value");

    core.commit();

    ASSERT_EQ(se->value(), 100);
    ASSERT_EQ(ae->value(), "value");
}
