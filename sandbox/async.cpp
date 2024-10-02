#include <nil/gate.hpp>

#include <nil/gate/bias/nil.hpp>
#include <nil/gate/runners/NonBlocking.hpp>

#include <iostream>

float deferred(const nil::gate::Core& core, nil::gate::async_outputs<int> z, bool a)
{
    std::cout << "deferred: " << a << std::endl;
    if (a)
    {
        auto [zz] = core.batch(z);
        // this will be triggered on next core.run()
        zz->set_value(zz->value() + 100);
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
    auto* a = core.edge(false);
    auto* l = core.edge(std::cref(ref));
    auto* r = core.edge(200.f);

    core.node(&bar, {a});

    const auto [f, x] = core.node(&deferred, {a});
    x->set_value(9000);

    const auto [fs] = core.node(&switcher, {a, l, r});
    core.node(printer_i, {x});

    core.node(printer_f, {fs});
    core.node(&foo, {x});

    core.set_runner<nil::gate::runners::NonBlocking>();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        a->set_value(!a->value());
        core.commit();
    }
}
