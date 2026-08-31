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
        expect(modes.effectiveBankMsb(9, 0) == 120,
               "GM and GS rhythm mode selects 2006LE's internal drum bank");
        expect(modes.effectiveBankMsb(9, 48) == 120,
               "GS bank variants retain 2026 drums when the kit exists");
        expect(modes.effectiveBankLsb(9, 7) == 0,
               "GM and GS rhythm mode clears melodic bank variation");
    }

    constexpr std::array<std::uint8_t, 11> demoGsReset {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7
    };
    modes.reset(hybrid::classifySystemReset(demoGsReset));
    expect(!modes.selectBankMsb(9, 0) && modes.isRhythm(9),
           "DEMO0002 GS Reset followed by bank zero keeps drums on channel 10");
    expect(modes.effectiveBankMsb(9, 0) == 120,
           "DEMO0002 GS Reset selects 2006LE's internal drum bank");

    constexpr std::array<std::uint8_t, 11> gsPart9Drum2 {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x19, 0x15, 0x02, 0x10, 0xf7
    };
    const auto part9Drum2 = modes.observe(gsPart9Drum2);
    expect(part9Drum2 && part9Drum2->part == 8 && part9Drum2->rhythm,
           "SC-88 Part 9 Drum2 SysEx enables rhythm on MIDI channel 9");
    expect(modes.effectiveBankMsb(8, 0) == 120,
           "SC-88 secondary rhythm part uses the internal drum bank");
    expect(modes.sharedRhythmMap(8) == 2,
           "SC-88 Drum2 assignment retains Roland Map 2 identity");
    expect(!modes.sharesRhythmMap(8, 9),
           "Roland Map 1 and Map 2 remain independent");

    constexpr std::array<std::uint8_t, 11> gsPart11Drum2 {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x1a, 0x15, 0x02, 0x0f, 0xf7
    };
    const auto part11Drum2 = modes.observe(gsPart11Drum2);
    expect(part11Drum2 && part11Drum2->part == 10,
           "SC-88 Part 11 Drum2 address maps to MIDI channel 11");
    expect(modes.sharesRhythmMap(8, 10),
           "parts assigned to Roland Drum2 share their kit state");

    constexpr std::array<std::uint8_t, 11> gsPart9Normal {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x19, 0x15, 0x00, 0x12, 0xf7
    };
    const auto part9Normal = modes.observe(gsPart9Normal);
    expect(part9Normal && part9Normal->part == 8 && !part9Normal->rhythm,
           "SC-88 Part 9 rhythm-off SysEx restores melodic mode");

    auto malformedGsPartMode = gsPart9Drum2;
    malformedGsPartMode[9] = 0;
    expect(!modes.observe(malformedGsPartMode),
           "SC-88 rhythm-part SysEx with a bad checksum is rejected");

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
