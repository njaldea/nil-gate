#pragma once

namespace nil::gate
{
    class Core;
    class IPort;
}

namespace nil::gate
{
    class INode
    {
    public:
        INode() = default;
        virtual ~INode() noexcept = default;

        INode(INode&&) noexcept = delete;
        INode& operator=(INode&&) noexcept = delete;

        INode(const INode&) = delete;
        INode& operator=(const INode&) = delete;

        virtual void run() = 0;
        virtual void exec() = 0;
        virtual void done() = 0;

        virtual bool is_input_changed() const = 0;
        virtual bool is_pending() const = 0;
        virtual bool is_ready() const = 0;

        virtual void pend() = 0;
        virtual void input_changed() = 0;

        virtual int score() const = 0;
        virtual void detach_in(IPort* port) = 0;

    protected:
        enum class ENodeState
        {
            Done = 0b0001,
            Pending = 0b0010
        };

        enum class EInputState
        {
            Stale = 0b0001,
            Changed = 0b0010
        };
    };
}
