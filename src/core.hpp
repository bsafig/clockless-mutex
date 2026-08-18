#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace clk
{
    using ProcessId = std::uint32_t;
    using Payload = std::string;
    using Bytes = std::vector<uint8_t>;
}