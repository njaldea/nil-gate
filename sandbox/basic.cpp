#include <nil/gate.hpp>
#include <nil/gate/runners/Immediate.hpp>

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
        std::cout << "calling      : " << tag << " " << (v + ... + 10) << std::endl;
        return (v + ... + 10);
    }

    std::string tag;
};

int main()
{
    nil::gate::runners::Immediate runner;
    nil::gate::Core core(&runner);

    using B = T<int(int, int)>;
    using C = T<int(int, int)>;
    using D = T<int(int, int)>;

    nil::gate::Port<int>* a1 = nullptr;
    nil::gate::Port<int>* a2 = nullptr;

    core.apply(
        [&a1, &a2](nil::gate::Graph& graph)
        {
            a1 = graph.port(0);
            a2 = graph.port(0);

            const auto [b1] = graph.node(B("b"), {a1, a2})->outputs();
            const auto [c1] = graph.node(C("c"), {b1, a2})->outputs();
            graph.node(D("d"), {c1, a2});

            auto outs = graph.unode<int>({
                .inputs = {c1},
                .req_output_size = 1,
                .opt_output_size = 1,
                .fn = [](const nil::gate::UNode<int>::Arg& v) -> std::vector<int>
                {
                    std::cout << __FILE__ << ':' << __LINE__ << std::endl;
                    std::cout << v.inputs.size() << ','      //
                              << v.opt_outputs.size() << ',' //
                              << v.req_outputs << std::endl;
                    return {100}; //
                } //
            });
            std::cout << __FILE__ << ':' << __LINE__ << std::endl;
            std::cout << outs.size() << std::endl;
        }
    );

    core.apply([=]() { a1->set_value(2); });

    core.apply(
        [&a1](nil::gate::Graph& graph)
        {
            graph.remove(a1);
            a1 = nullptr;
        }
    );
}
