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
