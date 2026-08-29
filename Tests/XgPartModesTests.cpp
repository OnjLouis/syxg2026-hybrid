#include "XgPartModes.h"

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
    hybrid::XgPartModes modes;
    expect(modes.isRhythm(9), "channel 10 defaults to rhythm");
    expect(!modes.isRhythm(5), "other channels default to melodic");
    expect(modes.effectiveBankMsb(9, 0) == 127,
           "default rhythm channel selects the drum bank");
    expect(modes.effectiveBankLsb(9, 113) == 0,
           "default rhythm channel clears melodic bank variation");
    expect(modes.effectiveBankMsb(5, 64) == 64,
           "melodic channels preserve the selected bank");
    expect(modes.effectiveBankLsb(5, 113) == 113,
           "melodic channels preserve the selected bank variation");

    const std::array<std::uint8_t, 9> enable {
        0xf0, 0x43, 0x10, 0x4c, 0x08, 0x05, 0x07, 0x02, 0xf7
    };
    const auto enabled = modes.observe(enable);
    expect(enabled && enabled->part == 5 && enabled->rhythm,
           "Yamaha part mode enables rhythm on an arbitrary channel");
    expect(modes.effectiveBankMsb(5, 0) == 127,
           "enabled rhythm channel selects the drum bank");
    expect(modes.effectiveBankLsb(5, 113) == 0,
           "enabled rhythm channel clears melodic bank variation");

    const std::array<std::uint8_t, 9> disable {
        0xf0, 0x43, 0x10, 0x4c, 0x08, 0x05, 0x07, 0x00, 0xf7
    };
    const auto disabled = modes.observe(disable);
    expect(disabled && disabled->part == 5 && !disabled->rhythm,
           "Yamaha part mode restores melodic operation");
    expect(modes.effectiveBankMsb(5, 64) == 64,
           "restored melodic channel uses its selected bank");
    expect(modes.effectiveBankLsb(5, 113) == 113,
           "restored melodic channel recovers its selected bank variation");

    const std::array<std::uint8_t, 9> unrelated {
        0xf0, 0x43, 0x10, 0x4c, 0x08, 0x05, 0x14, 0x7f, 0xf7
    };
    expect(!modes.observe(unrelated),
           "unrelated Yamaha part parameters are ignored");

    const std::array<std::uint8_t, 11> ranged {
        0xf0, 0x43, 0x10, 0x4c, 0x08, 0x04, 0x05, 0x00, 0x00, 0x01, 0xf7
    };
    const auto rangedChange = modes.observe(ranged);
    expect(rangedChange && rangedChange->part == 4 && rangedChange->rhythm,
           "multi-byte part parameters include part-mode changes");

    modes.reset();
    expect(modes.isRhythm(9) && !modes.isRhythm(5),
           "reset restores the XG rhythm-part default");
    return failures == 0 ? 0 : 1;
}
