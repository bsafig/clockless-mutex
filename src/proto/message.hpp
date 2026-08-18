#pragma once

#include "../core.hpp"

namespace clk
{
    struct message
    {
        ProcessId from;
        ProcessId to;
        LogicalTime timestamp;
        Payload payload;
    };
}