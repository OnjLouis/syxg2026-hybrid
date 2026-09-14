#include "XglVoiceMap.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace hybrid {
namespace {

constexpr std::uint8_t firstPanelBankLsb = 112;
constexpr std::uint8_t lastPanelBankLsb = 127;

} // namespace

XglVoiceMap::XglVoiceMap(std::span<const std::uint8_t> table)
{
    if (table.size() < requiredTableSize)
        throw std::runtime_error("sxgbnw6l.tbl is truncated");
    std::copy_n(table.begin() + bankMapOffset, banks.size(), banks.begin());
    const auto programBytes = table.subspan(programMapOffset, programMapSize);
    for (std::size_t index = 0; index < voices.size(); ++index) {
        voices[index] = static_cast<std::uint16_t>(programBytes[index * 2])
            | static_cast<std::uint16_t>(programBytes[index * 2 + 1] << 8);
    }
}

XglVoiceMap XglVoiceMap::load(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open sxgbnw6l.tbl");
    std::vector<std::uint8_t> table(
        std::istreambuf_iterator<char>(input), {});
    if (input.bad())
        throw std::runtime_error("cannot read sxgbnw6l.tbl");
    return XglVoiceMap(table);
}

bool XglVoiceMap::hasVoice(std::uint8_t bankMsb, std::uint8_t bankLsb,
                           std::uint8_t program) const noexcept
{
    const auto bank = banks[static_cast<std::size_t>(bankMsb) * 128 + bankLsb];
    if (bank == missingBank || bank >= compactBankCount)
        return false;
    return voices[static_cast<std::size_t>(bank) * programsPerBank + program]
        != missingVoice;
}

bool XglVoiceMap::shouldUse2006Engine(std::uint8_t bankMsb,
                                     std::uint8_t bankLsb,
                                     std::uint8_t program) const noexcept
{
    const auto bank = banks[static_cast<std::size_t>(bankMsb) * 128 + bankLsb];
    if (bank == missingBank || bank >= compactBankCount)
        return false;

    // Yamaha keyboard panel banks rely on S-YXG2006LE's own basic-voice
    // fallback when an exact panel slot is absent from its compact table.
    if (bankMsb == 0 && bankLsb >= firstPanelBankLsb
        && bankLsb <= lastPanelBankLsb) {
        return true;
    }
    return hasVoice(bankMsb, bankLsb, program);
}

} // namespace hybrid
