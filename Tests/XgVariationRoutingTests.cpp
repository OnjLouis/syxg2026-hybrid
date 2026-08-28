#include "XgVariationRouting.h"

#include <array>
#include <cstdint>
#include <cstdio>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

} // namespace

int main()
{
    hybrid::XgVariationRouting routing;
    expect(routing.connection()
               == hybrid::XgVariationConnection::insertion,
           "XG reset defaults variation to insertion");
    expect(!routing.assignedPart().has_value(),
           "XG reset leaves variation unassigned");

    const std::array<std::uint8_t, 9> system {
        0xf0, 0x43, 0x10, 0x4c, 0x02, 0x01, 0x5a, 0x01, 0xf7
    };
    routing.observe(system);
    expect(routing.connection() == hybrid::XgVariationConnection::system,
           "system connection is observed");

    const std::array<std::uint8_t, 10> insertionPart4 {
        0xf0, 0x43, 0x10, 0x4c, 0x02, 0x01, 0x5a, 0x00, 0x03, 0xf7
    };
    routing.observe(insertionPart4);
    expect(routing.connection()
               == hybrid::XgVariationConnection::insertion,
           "multi-byte update restores insertion connection");
    expect(routing.assignedPart() == 3,
           "multi-byte update assigns variation to part 4");
    expect(routing.isInsertionPart(3),
           "assigned part is routed through insertion variation");
    expect(!routing.isInsertionPart(2),
           "other parts do not enter insertion variation");

    const std::array<std::uint8_t, 9> off {
        0xf0, 0x43, 0x10, 0x4c, 0x02, 0x01, 0x5b, 0x7f, 0xf7
    };
    routing.observe(off);
    expect(!routing.assignedPart().has_value(),
           "part 127 disables insertion assignment");

    const std::array<std::uint8_t, 9> unrelated {
        0xf0, 0x43, 0x10, 0x4c, 0x02, 0x01, 0x40, 0x07, 0xf7
    };
    routing.observe(insertionPart4);
    routing.observe(unrelated);
    expect(routing.assignedPart() == 3,
           "unrelated variation parameters preserve assignment");

    const std::array<std::uint8_t, 8> malformed {
        0xf0, 0x43, 0x10, 0x4c, 0x02, 0x01, 0x5a, 0xf7
    };
    routing.observe(malformed);
    expect(routing.connection()
               == hybrid::XgVariationConnection::insertion,
           "malformed parameter change is ignored");
    expect(routing.assignedPart() == 3,
           "malformed parameter change preserves assignment");

    routing.reset();
    expect(routing.connection()
               == hybrid::XgVariationConnection::insertion,
           "reset restores insertion connection");
    expect(!routing.assignedPart().has_value(),
           "reset clears insertion assignment");

    return failures == 0 ? 0 : 1;
}
