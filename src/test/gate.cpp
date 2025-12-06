#include <nil/gate.hpp>
#include <nil/gate/runners/SoftBlocking.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(gate, create_port_uninit)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    core.post(
        [](nil::gate::Graph& graph)
        {
            auto* port = graph.port<int>();
            ASSERT_FALSE(port->has_value());

            port->set_value(1);
            ASSERT_TRUE(port->has_value());
            ASSERT_EQ(port->value(), 1);

            port->unset_value();
            ASSERT_FALSE(port->has_value());
        }
    );
    core.commit();
}

TEST(gate, create_port_init)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    core.post(
        [](nil::gate::Graph& graph)
        {
            auto* port = graph.port(1);
            ASSERT_TRUE(port->has_value());
            ASSERT_EQ(port->value(), 1);

            port->set_value(2);
            ASSERT_TRUE(port->has_value());
            ASSERT_EQ(port->value(), 2);

            port->unset_value();
            ASSERT_FALSE(port->has_value());
        }
    );
    core.commit();
}

TEST(gate, create_node_req_output)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    nil::gate::ports::ReadOnly<int>* req_out = nullptr;
    core.apply(
        [&req_out](nil::gate::Graph& graph)
        {
            auto* port = graph.port(1);
            ASSERT_TRUE(port->has_value());
            ASSERT_EQ(port->value(), 1);

            auto* node = graph.node([](int v) { return v + 1; }, {port});
            std::tie(req_out) = node->outputs();

            ASSERT_FALSE(req_out->has_value());
        }
    );

    ASSERT_TRUE(req_out->has_value());
    ASSERT_EQ(req_out->value(), 2);
}

TEST(gate, create_node_opt_output)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    nil::gate::Port<int>* port = nullptr;
    nil::gate::ports::ReadOnly<int>* opt_out = nullptr;
    core.post(
        [&port, &opt_out](nil::gate::Graph& graph)
        {
            port = graph.port(1);
            ASSERT_TRUE(port->has_value());
            ASSERT_EQ(port->value(), 1);

            auto* node = graph.node(
                [](nil::gate::Core& c, nil::gate::opt_outputs<int> o, int v)
                {
                    c.post(
                        [o, v]()
                        {
                            if (v % 2 == 0)
                            {
                                get<0>(o)->unset_value();
                            }
                            else
                            {
                                get<0>(o)->set_value(v + 1);
                            }
                        }
                    );
                },
                {port}
            );
            std::tie(opt_out) = node->outputs();

            ASSERT_FALSE(opt_out->has_value());
        }
    );

    core.commit();
    ASSERT_TRUE(port->has_value());
    ASSERT_EQ(port->value(), 1);
    ASSERT_FALSE(opt_out->has_value());

    core.commit();
    ASSERT_TRUE(port->has_value());
    ASSERT_EQ(port->value(), 1);
    ASSERT_TRUE(opt_out->has_value());
    ASSERT_EQ(opt_out->value(), 2);

    port->set_value(0);
    ASSERT_TRUE(port->has_value());
    ASSERT_EQ(port->value(), 0);
    ASSERT_TRUE(opt_out->has_value());
    ASSERT_EQ(opt_out->value(), 2);

    core.commit();
    ASSERT_TRUE(port->has_value());
    ASSERT_EQ(port->value(), 0);
    ASSERT_TRUE(opt_out->has_value()); // unchanged. to be applied on next cycle
    ASSERT_EQ(opt_out->value(), 2);

    core.commit();
    ASSERT_TRUE(port->has_value());
    ASSERT_EQ(port->value(), 0);
    ASSERT_FALSE(opt_out->has_value());
}

TEST(gate, create_node_opt_output_core_commit_in_node)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string_view, int, int)>> foo;

    nil::gate::Port<int>* port = nullptr;
    nil::gate::Port<int>* side_port = nullptr;
    nil::gate::ports::ReadOnly<int>* opt_out = nullptr;
    core.apply(
        [&foo, &port, &opt_out, &side_port](nil::gate::Graph& graph)
        {
            static constexpr auto sut_node
                = [](nil::gate::Core& c, nil::gate::opt_outputs<int> o, int v)
            {
                c.post(
                    [v, o]()
                    {
                        // NOLINTNEXTLINE
                        if (v % 2 == 0)
                        {
                            get<0>(o)->unset_value();
                        }
                        else
                        {
                            get<0>(o)->set_value(v + 1);
                        }
                    }
                );
            };
            port = graph.port(1);
            std::tie(opt_out) = graph.node(sut_node, {port})->outputs();
            side_port = graph.port(100);
            graph.node([&](int v1, int v2) { foo.Call("dep node", v1, v2); }, {opt_out, side_port});
        }
    );
    ASSERT_TRUE(port->has_value());
    ASSERT_EQ(port->value(), 1);
    ASSERT_FALSE(opt_out->has_value());

    {
        // on commit, all nodes should execute
        EXPECT_CALL(foo, Call("dep node", 2, 100)) //
            .Times(1)
            .RetiresOnSaturation();
        core.commit();
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 1);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // pend set, should have no changes
        port->set_value(0);
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 0);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // commit, node should unset the out port
        // dep node should not run
        core.commit();
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 0);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // side port changed, should have no change
        side_port->set_value(101);
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 0);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // commit, should have no change since dep node
        // should not run
        core.commit();
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 0);
        ASSERT_FALSE(opt_out->has_value());
    }

    {
        // set port to odd, changes not yet applied
        port->set_value(11);
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 11);
        ASSERT_FALSE(opt_out->has_value());
    }

    {
        core.commit();
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 11);
        ASSERT_FALSE(opt_out->has_value());
    }

    {
        side_port->unset_value();
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 11);
        ASSERT_FALSE(opt_out->has_value());
    }
    {
        // commit, dep node should not run
        // no changes to other ports/node
        core.commit();
        ASSERT_TRUE(port->has_value());
        ASSERT_EQ(port->value(), 11);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 12);
    }
}
