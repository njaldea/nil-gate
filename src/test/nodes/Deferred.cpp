#include <nil/gate.hpp>
#include <nil/gate/runners/SoftBlocking.hpp>

#include <nil/gate/nodes/Deferred.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(nodes, asynced_blank)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    core.post(
        [&mocked_fn](nil::gate::Graph& graph)
        {
            graph.node(nil::gate::nodes::Deferred([&]() { mocked_fn.Call("NODE"); })); //
        }
    );

    EXPECT_CALL(mocked_fn, Call("NODE")) //
        .Times(1)
        .RetiresOnSaturation();

    core.commit();
}

TEST(nodes, asynced_input)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    core.post(
        [&mocked_fn](nil::gate::Graph& graph)
        {
            auto* e = graph.port(std::string("value"));

            graph.node(
                nil::gate::nodes::Deferred(
                    [&](const std::string& v)
                    {
                        mocked_fn.Call("NODE");
                        mocked_fn.Call(v);
                    }
                ),
                {e}
            );
        }
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

    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    nil::gate::ports::ReadOnly<std::string>* e = nullptr;

    core.post(
        [&mocked_fn, &e](nil::gate::Graph& graph)
        {
            auto* node = graph.node(nil::gate::nodes::Deferred(
                [&]()
                {
                    mocked_fn.Call("NODE");
                    return std::string("value");
                }
            ));

            e = get<0>(node->outputs());
        }
    );
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

    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    nil::gate::ports::ReadOnly<std::string>* e = nullptr;

    core.post(
        [&mocked_fn, &e](nil::gate::Graph& graph)
        {
            auto* node = graph.node(nil::gate::nodes::Deferred(
                [&](nil::gate::opt_outputs<std::string> s)
                {
                    mocked_fn.Call("NODE");
                    get<0>(s)->set_value("value");
                }
            ));
            e = get<0>(node->outputs());
        }
    );

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

    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    nil::gate::ports::ReadOnly<int>* se = nullptr;
    nil::gate::ports::ReadOnly<std::string>* ae = nullptr;

    core.post(
        [&mocked_fn, &se, &ae](nil::gate::Graph& graph)
        {
            auto* node = graph.node(nil::gate::nodes::Deferred(
                [&](nil::gate::opt_outputs<std::string> s)
                {
                    mocked_fn.Call("NODE");
                    get<0>(s)->set_value("value");
                    return 100;
                }
            ));
            std::tie(se, ae) = node->outputs();
        }
    );

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
