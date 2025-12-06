#include <nil/gate.hpp>
#include <nil/gate/bias/nil.hpp>
#include <nil/gate/runners/NonBlocking.hpp>
#include <nil/gate/runners/Parallel.hpp>
#include <nil/gate/uniform_api.hpp>

#include <iostream>

float deferred(const nil::gate::Core& core, nil::gate::opt_outputs<int> z, bool a)
{
    std::cout << "deferred: " << a << std::endl;
    if (a)
    {
        {
            auto [zz] = core.batch(z);
            // this will be triggered on next core.run()
            zz->set_value(zz->value() + 100);
        }
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

void foo(int v)
{
    std::cout << "foo: " << v << std::endl;
}

int main()
{
    nil::gate::Core core;

    const auto printer_i = [](int v) { std::cout << "printer<int>: " << v << std::endl; };
    const auto printer_f = [](float v) { std::cout << "printer<float>: " << v << std::endl; };

    auto ref = 100.f;
    auto* a = core.port(false);
    auto* l = core.port(std::cref(ref));
    auto* r = core.port(200.f);

    add_node(core, &bar, {a});

    const auto [f, x] = add_node(core, &deferred, {a})->outputs();
    const auto [fs] = add_node(core, &switcher, {a, l, r})->outputs();
    core.node(printer_i, {x});

    core.node(printer_f, {fs});
    core.node(&foo, {x});

    nil::gate::runners::Parallel runner(5);
    core.set_runner(&runner);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        a->set_value(!a->value());
        core.commit();
    }
}
