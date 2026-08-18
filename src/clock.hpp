#pragma once

#include "./core.hpp"

#include <algorithm>
#include <mutex>

namespace clk
{
    class LamportClock
    {
    public:
        // IR1
        LogicalTime tick()
        {
            std::lock_guard<std::mutex> lock(mtx_);
            time_++;
            return time_;
        }

        // IR2
        LogicalTime update(LogicalTime t)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            time_ = std::max(time_, t) + 1;
            return time_;
        }

        LogicalTime getTime() const
        {
            std::lock_guard<std::mutex> lock(mtx_);
            return time_;
        }

    private:
        LogicalTime time_ = 0;
        mutable std::mutex mtx_;
    };
}