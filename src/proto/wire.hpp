#pragma once

#include "message.hpp"

#include <cstddef>
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

    inline void put_u64(Bytes &out, std::uint64_t v)
    {
        for (int shift = 56; shift >= 0; shift -= 8)
            out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xFF));
    }

    inline std::uint64_t get_u64(const Bytes &in, std::size_t offset)
    {
        std::uint64_t v = 0;
        for (std::size_t i = 0; i < 8; ++i)
            v = (v << 8) | static_cast<std::uint64_t>(in[offset + i]);
        return v;
    }

    // Wire layout:
    //   from u32 | to u32 | timestamp u64 | type u8 | len u32 | payload[len]
    //   offset 0        4              8       16        17        21
    inline Bytes encode(const message &m)
    {
        Bytes out;
        put_u32(out, m.from);
        put_u32(out, m.to);
        put_u64(out, m.timestamp);
        out.push_back(static_cast<std::uint8_t>(m.type));
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
        LogicalTime ts = get_u64(in, 8);
        MsgType type = static_cast<MsgType>(in[16]);
        std::uint32_t len = get_u32(in, 17);

        std::string payload;
        for (std::uint32_t i = 0; i < len; ++i)
        {
            payload.push_back(static_cast<char>(in[21 + i]));
        }

        return message{from, to, ts, payload, type};
    }
}
