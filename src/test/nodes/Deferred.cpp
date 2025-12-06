#include <nil/gate.hpp>
#include <nil/gate/runners/SoftBlocking.hpp>

#include <nil/gate/nodes/Deferred.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(nodes, asynced_blank)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;

    core.node(nil::gate::nodes::Deferred([&]() { mocked_fn.Call("NODE"); }));

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
    nil::gate::runners::SoftBlocking runner;
    core.set_runner(&runner);

    auto* e = core.port(std::string("value"));

    core.node(
        nil::gate::nodes::Deferred(
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

TEST(nodes, asynced_req_output) // failing
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;
    nil::gate::runners::SoftBlocking runner;
    core.set_runner(&runner);

    auto* node = core.node(nil::gate::nodes::Deferred(
        [&]()
        {
            mocked_fn.Call("NODE");
            return std::string("value");
        }
    ));

    const auto [e] = node->outputs();

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();

    // WILL SEG FAULT SINCE IT SHOULD NOT HAVE BEEN POPULATED YET
    // ASSERT_EQ(e->value(), "value");

    core.commit();

    ASSERT_EQ(e->value(), "value");
}

TEST(nodes, asynced_opt_output)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::Core core;
    nil::gate::runners::SoftBlocking runner;
    core.set_runner(&runner);

    auto* node = core.node(nil::gate::nodes::Deferred(
        [&](nil::gate::opt_outputs<std::string> s)
        {
            mocked_fn.Call("NODE");
            get<0>(s)->set_value("value");
        }
    ));
    const auto [e] = node->outputs();

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
    nil::gate::runners::SoftBlocking runner;
    core.set_runner(&runner);

    auto* node = core.node(nil::gate::nodes::Deferred(
        [&](nil::gate::opt_outputs<std::string> s)
        {
            mocked_fn.Call("NODE");
            get<0>(s)->set_value("value");
            return 100;
        }
    ));
    const auto [se, ae] = node->outputs();

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
