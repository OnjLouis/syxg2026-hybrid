#include "GsPartPitch.h"

#include <array>
#include <cstdio>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (condition)
        return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    ++failures;
}

} // namespace

int main()
{
    constexpr std::array<std::uint8_t, 11> suplexBass {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x12, 0x16, 0x34, 0x64, 0xf7
    };
    const auto bass = hybrid::gsPartKeyShift(suplexBass);
    expect(bass && bass->part == 1,
           "Suplex GS Part 2 key shift maps to MIDI channel 2");
    expect(bass && bass->rpnCoarseTune == 0x34,
           "Suplex minus-one-octave value is retained for RPN coarse tuning");

    constexpr std::array<std::uint8_t, 12> rangedPartParameter {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x12, 0x15,
        0x00, 0x34, 0x65, 0xf7
    };
    const auto ranged = hybrid::gsPartKeyShift(rangedPartParameter);
    expect(ranged && ranged->part == 1 && ranged->rpnCoarseTune == 0x34,
           "multi-byte GS part data can contain a key-shift parameter");

    auto badChecksum = suplexBass;
    badChecksum[9] = 0;
    expect(!hybrid::gsPartKeyShift(badChecksum),
           "a GS key-shift message with a bad checksum is rejected");

    auto outOfRange = suplexBass;
    outOfRange[8] = 0x27;
    outOfRange[9] = 0x71;
    expect(!hybrid::gsPartKeyShift(outOfRange),
           "a GS key-shift value outside Roland's documented range is rejected");

    constexpr std::array<std::uint8_t, 11> unrelated {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x12, 0x19, 0x64, 0x51, 0xf7
    };
    expect(!hybrid::gsPartKeyShift(unrelated),
           "unrelated GS part parameters are ignored");

    return failures == 0 ? 0 : 1;
}
