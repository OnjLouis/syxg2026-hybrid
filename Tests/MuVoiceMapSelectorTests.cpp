#include "MuVoiceMapSelector.h"

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
    hybrid::MuVoiceMapSelector selector;
    expect(selector.mode() == hybrid::MuVoiceMapMode::native,
           "the hybrid voice map remains the default");
    expect(selector.allows2006Voice(0, 0),
           "the default permits 2006LE basic-bank voices");

    const std::array<std::uint8_t, 9> basic {
        0xf0, 0x43, 0x10, 0x49, 0x00, 0x00, 0x12, 0x00, 0xf7
    };
    expect(selector.observe(basic), "MU Basic selection is recognized");
    expect(selector.mode() == hybrid::MuVoiceMapMode::basic,
           "MU Basic selection is retained");
    expect(!selector.allows2006Voice(0, 0),
           "MU Basic uses XG50 for bank 0/0");
    expect(selector.allows2006Voice(0, 1),
           "MU Basic does not alter variation banks");
    expect(selector.allows2006Voice(127, 0),
           "MU Basic does not alter drum banks");

    const std::array<std::uint8_t, 9> xgReset {
        0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00, 0xf7
    };
    expect(!selector.observe(xgReset), "XG reset is not a map selection");
    expect(selector.mode() == hybrid::MuVoiceMapMode::basic,
           "voice-map selection persists across resets");

    const std::array<std::uint8_t, 9> unsupported {
        0xf0, 0x43, 0x10, 0x49, 0x00, 0x00, 0x12, 0x02, 0xf7
    };
    expect(!selector.observe(unsupported),
           "nonstandard map value is not claimed");
    expect(selector.mode() == hybrid::MuVoiceMapMode::basic,
           "unsupported values preserve the selected map");

    const std::array<std::uint8_t, 9> native {
        0xf0, 0x43, 0x1f, 0x49, 0x00, 0x00, 0x12, 0x01, 0xf7
    };
    expect(selector.observe(native), "MU Native selection is recognized");
    expect(selector.mode() == hybrid::MuVoiceMapMode::native,
           "MU Native restores hybrid voice selection");
    expect(selector.allows2006Voice(0, 0),
           "MU Native permits 2006LE basic-bank voices");

    auto wrongModel = basic;
    wrongModel[3] = 0x4c;
    expect(!selector.observe(wrongModel), "wrong model ID is ignored");

    auto wrongAddress = basic;
    wrongAddress[6] = 0x11;
    expect(!selector.observe(wrongAddress), "wrong address is ignored");

    auto wrongDevice = basic;
    wrongDevice[2] = 0x20;
    expect(!selector.observe(wrongDevice), "wrong device byte is ignored");

    const std::array<std::uint8_t, 8> truncated {
        0xf0, 0x43, 0x10, 0x49, 0x00, 0x00, 0x12, 0xf7
    };
    expect(!selector.observe(truncated), "truncated selection is ignored");
    expect(selector.mode() == hybrid::MuVoiceMapMode::native,
           "malformed messages preserve the selected map");

    return failures == 0 ? 0 : 1;
}
