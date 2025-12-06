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

    Port<int>* a = nullptr;                          // input port
    Port<int>* loopback_port = nullptr;              // current processor output
    ports::ReadOnly<int>* out = nullptr;             // current processor output
    INode* proc_node = nullptr;                      // current processor (add/sub)
    Node<tlist<int>, tlist<>>* looper = nullptr;     // printer node
    Node<tlist<int>, tlist<>>* print_node = nullptr; // printer node

    auto inc = [](int v, int loopback)
    {
        std::printf("inc(%d) -> %d [%d]\n", v, v + 1, loopback);
        return v + 1;
    };
    auto dec = [](int v, int loopback)
    {
        std::printf("dec(%d) -> %d [%d]\n", v, v - 1, loopback);
        return v - 1;
    };
    auto printer = [](int v) { std::printf("out=%d\n", v); };

    // Ensure input exists and build default chain (add)
    core.apply(
        [&](Graph& g)
        {
            std::printf("[init] create input + add chain\n");
            a = g.port(0);
            loopback_port = g.port(10);
            auto* n = g.node(inc, {a, loopback_port});
            std::tie(out) = n->outputs();
            looper = g.link(out, loopback_port);
            proc_node = n;
            print_node = g.node(printer, {out});
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
                if (mode == "add")
                {
                    std::printf("[switch] add mode\n");
                    auto* n = g.node(inc, {a, loopback_port});
                    std::tie(out) = n->outputs();
                    looper = g.link(out, loopback_port);
                    proc_node = n;
                    get<0>(print_node->inputs()) = out;
                }
                else if (mode == "sub")
                {
                    std::printf("[switch] sub mode\n");
                    auto* n = g.node(dec, {a, loopback_port});
                    std::tie(out) = n->outputs();
                    looper = g.link(out, loopback_port);
                    proc_node = n;
                    get<0>(print_node->inputs()) = out;
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
                core.apply([&a, v]() { a->set_value(v); });
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
                core.apply([&a, v]() { a->set_value(v); });
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
