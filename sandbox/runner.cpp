#include <nil/gate.hpp>
#include <nil/gate/runners/Async.hpp>
#include <nil/gate/runners/Immediate.hpp>
#include <nil/gate/runners/boost_asio/Async.hpp>

#include <nil/gate/bias/nil.hpp>

#include <cstdio>
#include <iostream>
#include <thread>

int main()
{
    // nil::gate::runners::Immediate runner;
    nil::gate::runners::Async runner(1);
    // nil::gate::runners::boost_asio::Async runner(1);
    nil::gate::Core core(&runner);

    core.apply(
        [](nil::gate::Graph& graph)
        {
            auto* e1_i = graph.port(1);

            auto* main_node = graph.node(
                [](int v)
                {
                    std::printf("Main: %d\n", v);
                    return v + 10;
                },
                {e1_i}
            );
            const auto [e2_i] = main_node->outputs();

            auto* first_node = graph.node(
                [](int b1, int b2) { std::printf("First: %d : %d\n", b1, b2); },
                {e1_i, e2_i}
            );

            auto* second_node = graph.node(
                [e1_i](nil::gate::Core& c, nil::gate::opt_outputs<int> a, int b1, int b2)
                {
                    std::printf("Second: %d : %d\n", b1, b2);
                    c.post([=, e1_ii = e1_i->to_direct()]()
                           { e1_ii->set_value(e1_ii->value() + 1); });

                    if (b1 % 2 != 0)
                    {
                        c.post([aa = get<0>(a)]() { aa->set_value(aa->value() + 20); });
                    }
                },
                {e1_i, e2_i}
            );
            const auto [r] = second_node->outputs();

            auto* pr_node = graph.node([](int i) { std::printf("printer: %d\n", i); }, {r});

            std::cout << "mn node : " << main_node << std::endl;
            std::cout << "f1 node : " << first_node << std::endl;
            std::cout << "f2 node : " << second_node << std::endl;
            std::cout << "pr node : " << pr_node << std::endl;
            std::cout << "------" << std::endl;
        }
    );

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        {
            // auto [b] = core.batch(e1_i);
            // getting value here might not be thread safe.. so be ware.
            // b->set_value(b->value() + 1);
        }
        core.commit();
        static int count = 0;
        ++count;
        if (count == 5)
        {
            break;
        }
    }
}
