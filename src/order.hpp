#pragma once

#include "core.hpp"

namespace clk
{
    struct Stamp
    {
        LogicalTime time;
        ProcessId pid;
    };

    inline bool operator<(const Stamp &a, const Stamp &b)
    {
        return (a.time < b.time) || (a.time == b.time && a.pid < b.pid);
    }
}