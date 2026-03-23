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
            auto* port = graph.port<int>()->to_direct();
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
            auto* port = graph.port(1)->to_direct();
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
            ASSERT_TRUE(port->to_direct()->has_value());
            ASSERT_EQ(port->to_direct()->value(), 1);

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

    nil::gate::ports::External<int>* port = nullptr;
    nil::gate::ports::ReadOnly<int>* opt_out = nullptr;
    core.post(
        [&port, &opt_out](nil::gate::Graph& graph)
        {
            port = graph.port(1);
            ASSERT_TRUE(port->to_direct()->has_value());
            ASSERT_EQ(port->to_direct()->value(), 1);

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
    ASSERT_TRUE(port->to_direct()->has_value());
    ASSERT_EQ(port->to_direct()->value(), 1);
    ASSERT_FALSE(opt_out->has_value());

    core.commit();
    ASSERT_TRUE(port->to_direct()->has_value());
    ASSERT_EQ(port->to_direct()->value(), 1);
    ASSERT_TRUE(opt_out->has_value());
    ASSERT_EQ(opt_out->value(), 2);

    core.post([mport = port->to_direct()]() { mport->set_value(0); });
    ASSERT_TRUE(port->to_direct()->has_value());
    ASSERT_EQ(port->to_direct()->value(), 1);
    ASSERT_TRUE(opt_out->has_value());
    ASSERT_EQ(opt_out->value(), 2);

    core.commit();
    ASSERT_TRUE(port->to_direct()->has_value());
    ASSERT_EQ(port->to_direct()->value(), 0);
    ASSERT_TRUE(opt_out->has_value()); // unchanged. to be applied on next cycle
    ASSERT_EQ(opt_out->value(), 2);

    core.commit();
    ASSERT_TRUE(port->to_direct()->has_value());
    ASSERT_EQ(port->to_direct()->value(), 0);
    ASSERT_FALSE(opt_out->has_value());
}

TEST(gate, create_node_opt_output_core_commit_in_node)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    const testing::InSequence seq;
    testing::StrictMock<testing::MockFunction<void(std::string_view, int, int)>> foo;

    nil::gate::ports::External<int>* port = nullptr;
    nil::gate::ports::External<int>* side_port = nullptr;
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
    ASSERT_TRUE(port->to_direct()->has_value());
    ASSERT_EQ(port->to_direct()->value(), 1);
    ASSERT_FALSE(opt_out->has_value());

    {
        // on commit, all nodes should execute
        EXPECT_CALL(foo, Call("dep node", 2, 100)) //
            .Times(1)
            .RetiresOnSaturation();
        core.commit();
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 1);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // pend set, should have no changes
        core.post([mport = port->to_direct()]() { mport->set_value(0); });
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 1);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // commit, node should unset the out port
        // dep node should not run
        core.commit();
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 0);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // side port changed, should have no change
        core.post([mport = side_port->to_direct()]() { mport->set_value(101); });
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 0);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 2);
    }

    {
        // commit, should have no change since dep node
        // should not run
        core.commit();
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 0);
        ASSERT_FALSE(opt_out->has_value());
    }

    {
        // set port to odd, changes not yet applied
        core.post([mport = port->to_direct()]() { mport->set_value(11); });
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 0);
        ASSERT_FALSE(opt_out->has_value());
    }

    {
        core.commit();
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 11);
        ASSERT_FALSE(opt_out->has_value());
    }

    {
        core.post([mside_port = side_port->to_direct()]() { mside_port->unset_value(); });
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 11);
        ASSERT_FALSE(opt_out->has_value());
    }
    {
        // commit, dep node should not run
        // no changes to other ports/node
        core.commit();
        ASSERT_TRUE(port->to_direct()->has_value());
        ASSERT_EQ(port->to_direct()->value(), 11);
        ASSERT_TRUE(opt_out->has_value());
        ASSERT_EQ(opt_out->value(), 12);
    }
}

TEST(gate, link_set_value_in_post)
{
    nil::gate::runners::SoftBlocking runner;
    nil::gate::Core core(&runner);

    nil::gate::ports::External<int>* src_port = nullptr;
    nil::gate::ports::External<int>* dst_port = nullptr;
    nil::gate::ports::ReadOnly<int>* out = nullptr;

    core.apply(
        [&src_port, &dst_port, &out](nil::gate::Graph& graph)
        {
            src_port = graph.port(1);
            dst_port = graph.port<int>();
            graph.link(src_port->to_direct(), dst_port);
            auto* node = graph.node([](int v) { return v * 2; }, {dst_port});
            std::tie(out) = node->outputs();
        }
    );

    // After apply, the link node ran and posted set_value(1) to dst_port via core.post.
    // dst_port is not yet updated because set_value is called inside post, not directly.
    ASSERT_TRUE(src_port->to_direct()->has_value());
    ASSERT_EQ(src_port->to_direct()->value(), 1);
    ASSERT_FALSE(dst_port->to_direct()->has_value());
    ASSERT_FALSE(out->has_value());

    // Second commit applies the posted set_value. dst_port gets its value and downstream node runs.
    core.commit();
    ASSERT_TRUE(dst_port->to_direct()->has_value());
    ASSERT_EQ(dst_port->to_direct()->value(), 1);
    ASSERT_TRUE(out->has_value());
    ASSERT_EQ(out->value(), 2);

    // Change src_port value; link node posts set_value(3) for dst_port.
    core.post([mport = src_port->to_direct()]() { mport->set_value(3); });
    core.commit();

    // After this commit, src changed and link ran, but dst_port still holds the old value
    // because the set_value(3) for dst_port was posted, not yet applied.
    ASSERT_EQ(src_port->to_direct()->value(), 3);
    ASSERT_EQ(dst_port->to_direct()->value(), 1);
    ASSERT_EQ(out->value(), 2);

    // Next commit applies the posted set_value(3) to dst_port; downstream node re-runs.
    core.commit();
    ASSERT_EQ(dst_port->to_direct()->value(), 3);
    ASSERT_EQ(out->value(), 6);
}
