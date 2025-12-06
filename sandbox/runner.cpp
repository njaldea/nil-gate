#include <nil/gate.hpp>
#include <nil/gate/runners/NonBlocking.hpp>
#include <nil/gate/runners/Parallel.hpp>
#include <nil/gate/runners/boost_asio/Parallel.hpp>
#include <nil/gate/runners/boost_asio/Serial.hpp>

#include <nil/gate/bias/nil.hpp>

#include <cstdio>
#include <thread>

int main()
{
    nil::gate::Core core;

    auto* e1_i = core.port(0);

    auto* node1 = core.node(
        [](int v)
        {
            std::printf("Main: %d\n", v);
            return v + 10;
        },
        {e1_i}
    );
    auto [e2_i] = node1->outputs();

    core.node([](int b1, int b2) { std::printf("First: %d : %d\n", b1, b2); }, {e1_i, e2_i});

    auto* node2 = core.node(
        [](const nil::gate::Core& c, nil::gate::opt_outputs<int> a, int b1, int b2)
        {
            std::printf("Second: %d : %d\n", b1, b2);
            if (b1 % 2 == 0)
            {
                auto [aa] = c.batch(a);
                aa->set_value(aa->value() + 20);
            }
        },
        {e1_i, e2_i}
    );
    const auto [r] = node2->outputs();

    core.node([](int i) { std::printf("printer: %d\n", i); }, {r});

    nil::gate::runners::NonBlocking non_blocking_runner;
    nil::gate::runners::Parallel parallel_runner(10);
    nil::gate::runners::boost_asio::Serial basio_serial_runner;
    nil::gate::runners::boost_asio::Parallel baso_parallel_runner(10);
    core.set_runner(&non_blocking_runner);
    core.set_runner(&parallel_runner);
    core.set_runner(&basio_serial_runner);
    core.set_runner(&baso_parallel_runner);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        {
            auto [b] = core.batch(e1_i);
            // getting value here might not be thread safe.. so be ware.
            b->set_value(b->value() + 1);
        }
        core.commit();
    }
}
