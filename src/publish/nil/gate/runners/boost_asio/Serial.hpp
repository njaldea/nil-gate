#pragma once

// This header is only available for use if the user has actual dependency on boost asio

#include "../../IRunner.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <thread>

namespace nil::gate::runners::boost_asio
{
    class Serial final: public nil::gate::IRunner
    {
    public:
        explicit Serial()
            : main_work(boost::asio::make_work_guard(main_context))
            , main_th([this]() { main_context.run(); })
        {
        }

        ~Serial() noexcept override
        {
            main_context.stop();
            main_work.reset();
            main_th.join();
        }

        Serial(Serial&&) = delete;
        Serial(const Serial&) = delete;
        Serial& operator=(Serial&&) = delete;
        Serial& operator=(const Serial&) = delete;

        void flush(std::unique_ptr<nil::gate::ICallable<void()>> invokable) override
        {
            boost::asio::post(
                main_context,
                [invokable = std::move(invokable)]() mutable
                {
                    if (invokable)
                    {
                        invokable->call();
                    }
                }
            );
        }

        void run(const std::vector<std::unique_ptr<nil::gate::INode>>& nodes) override
        {
            boost::asio::post(
                main_context,
                [&nodes]()
                {
                    for (const auto& node : nodes)
                    {
                        node->run();
                    }
                }
            );
        }

    private:
        boost::asio::io_context main_context;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> main_work;

        std::thread main_th;
    };
}
