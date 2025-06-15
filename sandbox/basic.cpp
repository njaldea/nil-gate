#include <nil/gate.hpp>

#include <nil/gate/bias/nil.hpp>

#include <iostream>

template <typename U>
struct T;

template <typename... A>
struct T<void(A...)>: T<std::tuple<>(A...)>
{
    using T<std::tuple<>(A...)>::T;
};

template <typename R, typename... A>
struct T<R(A...)>
{
    explicit T(std::string init_tag)
        : tag(std::move(init_tag))
    {
        std::cout << "constructing : " << tag << std::endl;
    }

    R operator()(A... v) const
    {
        std::cout << "calling      : " << tag << std::endl;
        return (v + ... + 10);
    }

    std::string tag;
};

int main()

{
    nil::gate::Core core;

    using B = T<int(int, int)>;
    using C = T<int(int, int)>;
    using D = T<int(int, int)>;

    auto* a1 = core.port(int(0));
    auto* a2 = core.port(int(0));

    const auto [b1] = core.node(B("b"), {a1, a2});
    const auto [c1] = core.node(C("c"), {b1, a2});
    core.node(D("d"), {c1, a2});

    std::cout << __FILE__ << ':' << __LINE__ << ':' << (const char*)(__FUNCTION__) << std::endl;
    core.commit();

    a2->set_value(2);

    std::cout << __FILE__ << ':' << __LINE__ << ':' << (const char*)(__FUNCTION__) << std::endl;
    core.commit();

    a2->set_value(4);

    std::cout << __FILE__ << ':' << __LINE__ << ':' << (const char*)(__FUNCTION__) << std::endl;
    core.commit();
}
