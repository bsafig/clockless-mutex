#include "../src/proto/wire.hpp"

#include <cassert>
#include <cstdio>
#include <string>

using namespace clk;

static void check_roundtrip(const message &m)
{
    Bytes b = encode(m);
    message r = decode(b);
    assert(r.from == m.from);
    assert(r.to == m.to);
    assert(r.payload == m.payload);
    printf("  ok: from=%u to=%u payloadLen=%zu (%zu bytes on wire)\n",
           m.from, m.to, m.payload.size(), b.size());
}

int main()
{
    printf("wire round-trip tests:\n");
    check_roundtrip(message{7, 42, "hello"});
    check_roundtrip(message{0, 0, ""});
    check_roundtrip(message{1, 4294967295u, "x"});
    check_roundtrip(message{100, 200, std::string("a\0b", 3)});
    printf("all wire tests passed\n");
    return 0;
}
