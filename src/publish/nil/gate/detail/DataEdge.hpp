#pragma once

#include "../Diffs.hpp"
#include "../INode.hpp"
#include "../edges/Mutable.hpp"

#include <optional>
#include <vector>

namespace nil::gate::detail::edges
{
    /**
     * @brief Edge type returned by Core::edge.
     *  For internal use.
     *
     * @tparam T
     */
    template <typename T>
    class Data final: public nil::gate::edges::Mutable<T>
    {
    public:
        Data()
            : nil::gate::edges::Mutable<T>()
            , state(EState::Pending)
            , data(std::nullopt)
            , diffs(nullptr)
        {
        }

        explicit Data(T init_data)
            : nil::gate::edges::Mutable<T>()
            , state(EState::Stale)
            , data(std::make_optional<T>(std::move(init_data)))
            , diffs(nullptr)
        {
        }

        ~Data() noexcept override = default;

        Data(Data&&) noexcept = delete;
        Data& operator=(Data&&) noexcept = delete;

        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

        const T& value() const override
        {
            return *data;
        }

        void set_value(T new_data) override
        {
            diffs->push(make_callable(
                [this, new_data = std::move(new_data)]() mutable
                {
                    if (exec(std::move(new_data)))
                    {
                        pend();
                        done();
                    }
                }
            ));
        }

        bool exec(T new_data)
        {
            if (!data.has_value() || !(data.value() == new_data))
            {
                data.emplace(std::move(new_data));
                return true;
            }
            return false;
        }

        void pend()
        {
            if (state != EState::Pending)
            {
                state = EState::Pending;
                for (auto* out : this->outs)
                {
                    out->pend();
                }
            }
        }

        void done()
        {
            state = EState::Stale;
        }

        void attach(INode* node)
        {
            outs.push_back(node);
        }

        void attach(Diffs* new_diffs)
        {
            diffs = new_diffs;
        }

        bool is_ready() const
        {
            return state != EState::Pending && data.has_value();
        }

        nil::gate::edges::Mutable<T>* as_mutable()
        {
            return this;
        }

        nil::gate::edges::ReadOnly<T>* as_readonly()
        {
            return this;
        }

    private:
        enum class EState
        {
            Stale = 0b0001,
            Pending = 0b0010
        };

        EState state;
        std::optional<T> data;
        nil::gate::Diffs* diffs;
        std::vector<INode*> outs;
    };
}
