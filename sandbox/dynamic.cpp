#include <nil/gate.hpp>
#include <nil/gate/runners/Immediate.hpp>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

int main() // NOLINT
{
    using namespace nil::gate;
    using nil::xalt::tlist;

    runners::Immediate runner;
    Core core(&runner);

    ports::External<int>* a = nullptr;             // input port
    ports::External<int>* loopback_port = nullptr; // current processor output
    Node<nil::xalt::tlist<int, int>, nil::xalt::tlist<int>>* proc_node
        = nullptr;                                   // current processor (add/sub)
    Node<tlist<int>, tlist<>>* looper = nullptr;     // printer node
    Node<tlist<int>, tlist<>>* print_node = nullptr; // printer node

    auto inc = [](int v, int loopback)
    {
        std::printf("inc(%d) -> %d [%d]\n", v, v + loopback, loopback);
        return v + loopback;
    };

    auto dec = [](int v, int loopback)
    {
        std::printf("dec(%d) -> %d [%d]\n", v, v - loopback, loopback);
        return v - loopback;
    };

    auto printer = [](int v) { std::printf("out=%d\n", v); };

    // Ensure input exists and build default chain (add)
    core.apply(
        [&](Graph& g)
        {
            std::printf("[init] create input + add chain\n");
            a = g.port(0);
            loopback_port = g.port(10);

            proc_node = g.node(inc, {a, loopback_port});
            looper = g.link(get<0>(proc_node->outputs()), loopback_port);
            print_node = g.node(printer, {get<0>(proc_node->outputs())});
        }
    );

    auto switch_to = [&](const std::string& mode)
    {
        core.apply(
            [&](Graph& g)
            {
                if (proc_node != nullptr)
                {
                    g.remove(proc_node);
                    proc_node = nullptr;
                }
                if (looper != nullptr)
                {
                    g.remove(looper);
                    looper = nullptr;
                }
                if (print_node != nullptr)
                {
                    g.remove(print_node);
                    print_node = nullptr;
                }
                if (mode == "add")
                {
                    std::printf("[switch] add mode\n");
                    proc_node = g.node(inc, {a, loopback_port});
                    looper = g.link(get<0>(proc_node->outputs()), loopback_port);
                    print_node = g.node(printer, {get<0>(proc_node->outputs())});
                }
                else if (mode == "sub")
                {
                    std::printf("[switch] sub mode\n");
                    proc_node = g.node(dec, {a, loopback_port});
                    looper = g.link(get<0>(proc_node->outputs()), loopback_port);
                    print_node = g.node(printer, {get<0>(proc_node->outputs())});
                }
            }
        );
    };

    const auto help = []()
    {
        std::cout << "Interactive demo. Commands:\n"
                     "  add            -> switch to increment node\n"
                     "  sub            -> switch to decrement node\n"
                     "  set <n>        -> set input to n and run\n"
                     "  <n>            -> shorthand for set <n>\n"
                     "  commit         -> force-rerun\n"
                     "  quit           -> exit\n";
    };

    help();

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line))
    {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty())
        {
            continue;
        }

        if (cmd == "quit" || cmd == "q")
        {
            break;
        }

        if (cmd == "help" || cmd == "h")
        {
            help();
        }
        else if (cmd == "add" || cmd == "sub")
        {
            switch_to(cmd);
        }
        else if (cmd == "commit" || cmd == "c")
        {
            core.commit();
        }
        else if (cmd == "set")
        {
            int v = 0;
            if (iss >> v)
            {
                std::printf("[input] a=%d\n", v);
                a->set_value(v);
                core.commit();
            }
            else
            {
                std::cout << "usage: set <int>\n";
            }
        }
        else
        {
            // try parse as number shorthand
            std::istringstream issn(line);
            int v = 0;
            if (issn >> v)
            {
                std::printf("[input] a=%d\n", v);
                a->set_value(v);
                core.commit();
            }
            else
            {
                std::cout << "unknown command. try: add | sub | set <n> | quit\n";
            }
        }
    }

    std::printf("[done]\n");
    return 0;
}
