#include "XglVoiceMap.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

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
    std::vector<std::uint8_t> table(hybrid::XglVoiceMap::requiredTableSize,
                                    0xff);
    const auto bankSlot = hybrid::XglVoiceMap::bankMapOffset + 40;
    table[bankSlot] = 0;
    const auto voiceSlot = hybrid::XglVoiceMap::programMapOffset;
    table[voiceSlot] = 0x34;
    table[voiceSlot + 1] = 0x12;

    const hybrid::XglVoiceMap map(table);
    expect(map.hasVoice(0, 40, 0), "defined compact-bank voice is present");
    expect(!map.hasVoice(0, 40, 1), "missing compact-bank voice is absent");
    expect(!map.hasVoice(0, 41, 0), "missing bank is absent");
    expect(map.shouldUse2006Engine(0, 40, 0),
           "defined voice selects S-YXG2006LE");
    expect(!map.shouldUse2006Engine(0, 40, 1),
           "missing ordinary variation falls back to S-YXG50");

    table[hybrid::XglVoiceMap::bankMapOffset + 112] = 1;
    table[hybrid::XglVoiceMap::bankMapOffset + 127] = 2;
    const hybrid::XglVoiceMap panelMap(table);
    expect(panelMap.shouldUse2006Engine(0, 112, 4),
           "missing panel voice uses S-YXG2006LE native fallback");
    expect(panelMap.shouldUse2006Engine(0, 127, 127),
           "last panel bank uses S-YXG2006LE native fallback");
    expect(!panelMap.shouldUse2006Engine(0, 126, 4),
           "an undefined panel bank does not bypass the table");
    expect(!panelMap.shouldUse2006Engine(0, 111, 4),
           "bank below the panel range keeps ordinary fallback");
    expect(!panelMap.shouldUse2006Engine(1, 112, 4),
           "panel LSB on another MSB keeps ordinary fallback");

    table[hybrid::XglVoiceMap::bankMapOffset + 41]
        = hybrid::XglVoiceMap::compactBankCount;
    const hybrid::XglVoiceMap invalidBankMap(table);
    expect(!invalidBankMap.hasVoice(0, 41, 0),
           "out-of-range compact bank is rejected");

    bool rejectedTruncated = false;
    try {
        const hybrid::XglVoiceMap truncated(
            std::span<const std::uint8_t>(table).first(table.size() - 1));
        (void)truncated;
    } catch (const std::runtime_error&) {
        rejectedTruncated = true;
    }
    expect(rejectedTruncated, "truncated table is rejected");
    return failures == 0 ? 0 : 1;
}
