#include <nil/gate.hpp>
#include <nil/gate/runners/Immediate.hpp>

#include <nil/gate/nodes/Scoped.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(nodes, scoped_blank)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::runners::Immediate runner;
    nil::gate::Core core(&runner);

    core.post(
        [&mocked_fn](nil::gate::Graph& graph)
        {
            graph.node(nil::gate::nodes::Scoped(
                [&]() { mocked_fn.Call("PRE"); },
                [&]() { mocked_fn.Call("NODE"); },
                [&]() { mocked_fn.Call("POST"); }
            ));
        }
    );

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

TEST(nodes, scoped_with_req_out)
{
    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string)>> mocked_fn;

    nil::gate::runners::Immediate runner;
    nil::gate::Core core(&runner);

    nil::gate::ports::ReadOnly<int>* e = nullptr;
    core.post(
        [&e, &mocked_fn](nil::gate::Graph& graph)
        {
            auto* node = graph.node(nil::gate::nodes::Scoped(
                [&]() { mocked_fn.Call("PRE"); },
                [&]()
                {
                    mocked_fn.Call("NODE");
                    return 100;
                },
                [&]() { mocked_fn.Call("POST"); }
            ));
            std::tie(e) = node->outputs();
        }
    );

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
