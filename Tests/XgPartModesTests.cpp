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
    const auto melodicChannel10 = modes.selectBankMsb(9, 0);
    expect(melodicChannel10 && !melodicChannel10->rhythm,
           "channel 10 reports its bank-selected melodic transition");
    expect(!modes.isRhythm(9),
           "an explicit melodic bank releases channel 10 from its default rhythm mode");
    expect(modes.effectiveBankMsb(9, 0) == 0,
           "channel 10 preserves an explicit melodic bank MSB");
    expect(modes.effectiveBankLsb(9, 115) == 115,
           "channel 10 preserves an explicit melodic bank LSB");
    const auto drumChannel10 = modes.selectBankMsb(9, 127);
    expect(drumChannel10 && drumChannel10->rhythm,
           "drum bank 127 restores channel 10 rhythm mode");
    expect(modes.effectiveBankLsb(9, 115) == 0,
           "restored rhythm mode clears melodic bank variation");
    (void)modes.selectBankMsb(9, 0);
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
    (void)modes.selectBankMsb(5, 0);
    expect(modes.isRhythm(5),
           "an explicit rhythm-part command takes precedence over bank select");

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

    for (const auto system : { hybrid::MidiSystemReset::gm1,
                               hybrid::MidiSystemReset::gs }) {
        modes.reset(system);
        const auto gsStyleBankSelect = modes.selectBankMsb(9, 0);
        expect(!gsStyleBankSelect && modes.isRhythm(9),
               "GM and GS bank-zero setup keeps channel 10 in rhythm mode");
    }

    constexpr std::array<std::uint8_t, 11> demoGsReset {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7
    };
    modes.reset(hybrid::classifySystemReset(demoGsReset));
    expect(!modes.selectBankMsb(9, 0) && modes.isRhythm(9),
           "DEMO0002 GS Reset followed by bank zero keeps drums on channel 10");

    modes.reset(hybrid::MidiSystemReset::gm2);
    expect(!modes.selectBankMsb(9, 0) && modes.isRhythm(9),
           "GM2 bank zero setup keeps channel 10 in rhythm mode");
    expect(modes.effectiveBankMsb(9, 0) == 120,
           "GM2 rhythm mode selects the GM2 drum bank");
    expect(modes.selectBankMsb(9, 121) && !modes.isRhythm(9),
           "GM2 melodic bank releases channel 10 from rhythm mode");
    expect(modes.selectBankMsb(5, 120) && modes.isRhythm(5),
           "GM2 drum bank enables rhythm on another channel");

    modes.reset(hybrid::MidiSystemReset::xg);
    expect(modes.selectBankMsb(9, 0) && !modes.isRhythm(9),
           "XG bank selection can still make channel 10 melodic");
    return failures == 0 ? 0 : 1;
}
