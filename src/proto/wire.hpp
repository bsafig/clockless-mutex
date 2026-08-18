#pragma once

#include "message.hpp"
#include <cstdint>

namespace clk
{
    inline void put_u32(Bytes &out, std::uint32_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    inline std::uint32_t get_u32(const Bytes &in, std::size_t offset)
    {
        return (static_cast<std::uint32_t>(in[offset]) << 24) |
               (static_cast<std::uint32_t>(in[offset + 1]) << 16) |
               (static_cast<std::uint32_t>(in[offset + 2]) << 8) |
               (static_cast<std::uint32_t>(in[offset + 3]));
    }

    inline Bytes encode(const message &m)
    {
        Bytes out;
        put_u32(out, m.from);
        put_u32(out, m.to);
        put_u32(out, static_cast<std::uint32_t>(m.payload.size()));
        for (char c : m.payload)
        {
            out.push_back(static_cast<std::uint8_t>(c));
        }
        return out;
    }

    inline message decode(const Bytes &in)
    {
        std::uint32_t from = get_u32(in, 0);
        std::uint32_t to = get_u32(in, 4);
        std::uint32_t len = get_u32(in, 8);

        std::string payload;
        for (std::uint32_t i = 0; i < len; ++i)
        {
            payload.push_back(static_cast<char>(in[12 + i]));
        }

        return message{from, to, payload};
    }
}