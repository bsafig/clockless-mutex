#pragma once

#include "../core.hpp"

namespace clk
{
    enum class MsgType : std::uint8_t
    {
        App = 0,
        Request = 1,
        Ack = 2,
        Release = 3
    };
    struct message
    {
        ProcessId from;
        ProcessId to;
        LogicalTime timestamp;
        Payload payload;
        MsgType type = MsgType::App;
    };
}