#pragma once

// This header is only available for use if the user has actual dependency on boost asio

#include "../../IRunner.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <thread>

namespace nil::gate::runners::boost_asio
{
    class Serial final: public IRunner
    {
    public:
        explicit Serial()
            : main_th(
                  [this]()
                  {
                      auto _ = boost::asio::make_work_guard(main_context);
                      main_context.run();
                  }
              )
        {
        }

        ~Serial() noexcept override
        {
            main_context.stop();
            main_th.join();
        }

        Serial(Serial&&) = delete;
        Serial(const Serial&) = delete;
        Serial& operator=(Serial&&) = delete;
        Serial& operator=(const Serial&) = delete;

        void run(Core* core, std::function<void()> apply_changes, std::span<INode* const> nodes)
            override
        {
            boost::asio::post(
                main_context,
                [apply_changes = std::move(apply_changes), core, nodes]()
                {
                    if (apply_changes)
                    {
                        apply_changes();
                    }
                    for (const auto& node : nodes)
                    {
                        if (nullptr != node)
                        {
                            node->run(core);
                        }
                    }
                }
            );
        }

    private:
        boost::asio::io_context main_context;

        std::thread main_th;
    };
}
