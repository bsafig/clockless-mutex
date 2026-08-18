#pragma once

#include "../proto/message.hpp"
#include <optional>

namespace clk
{
    class ITransport
    {
    public:
        virtual ~ITransport() = default;

        virtual void send(const message &m) = 0;
        virtual std::optional<message> recv() = 0;
        virtual void close() = 0;
    };
}