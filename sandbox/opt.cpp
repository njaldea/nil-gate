#include "nil/gate/ports/Mutable.hpp"
#include <nil/gate.hpp>
#include <nil/gate/bias/nil.hpp>
#include <nil/gate/runners/NonBlocking.hpp>
#include <nil/gate/runners/Parallel.hpp>

#include <iostream>

float deferred(nil::gate::Core& core, nil::gate::opt_outputs<> /* z */, bool a)
{
    std::cout << "deferred: " << a << std::endl;
    if (a)
    {
        core.commit();
        return 321.0f;
    }
    return 432.0f;
}

std::reference_wrapper<const float> switcher(bool a, const float& l, const float& r)
{
    return a ? std::ref(l) : std::ref(r);
}

void bar(std::reference_wrapper<const bool> a)
{
    std::cout << a.get() << std::endl;
}

void foo(float v)
{
    std::cout << "foo: " << v << std::endl;
}

constexpr auto printer_i = [](float v) { std::cout << "printer<int>: " << v << std::endl; };
constexpr auto printer_f = [](float v) { std::cout << "printer<float>: " << v << std::endl; };

int main()
{
    nil::gate::runners::Parallel runner(5);
    nil::gate::Core core(&runner);

    auto ref = 100.f;
    nil::gate::Port<bool>* a = nullptr;

    core.apply(
        [&ref, &a](nil::gate::Graph& graph)
        {
            a = graph.port(false);
            auto* l = graph.port(std::cref(ref));
            auto* r = graph.port(200.f);

            graph.node(&bar, {a});

            const auto [f] = graph.node(&deferred, {a})->outputs();
            const auto [fs] = graph.node(&switcher, {a, l, r})->outputs();
            graph.node(printer_i, {f});
            graph.node(printer_f, {fs});
            graph.node(&foo, {f});
        }
    );

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        a->set_value(!a->value());
        core.commit();
    }
}
