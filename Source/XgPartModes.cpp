#include "XgPartModes.h"

namespace hybrid {

namespace {

std::optional<std::size_t> rolandGsPartIndex(std::uint8_t block) noexcept
{
    if (block == 0x10)
        return 9;
    if (block >= 0x11 && block <= 0x19)
        return static_cast<std::size_t>(block - 0x11);
    if (block >= 0x1a && block <= 0x1f)
        return static_cast<std::size_t>(block - 0x10);
    return std::nullopt;
}

} // namespace

void XgPartModes::reset(MidiSystemReset system) noexcept
{
    rhythmMaps.fill(0);
    rhythmMaps[defaultRhythmPart] = 1;
    explicitPartModes.fill(false);
    systemMode = system == MidiSystemReset::none
        ? MidiSystemReset::xg : system;
}

std::optional<XgPartModeChange> XgPartModes::observe(
    std::span<const std::uint8_t> sysex) noexcept
{
    constexpr std::size_t rolandAddressHigh = 5;
    constexpr std::size_t rolandPartBlock = 6;
    constexpr std::size_t rolandAddressLow = 7;
    constexpr std::size_t rolandData = 8;
    constexpr std::size_t rolandChecksum = 9;
    constexpr std::uint8_t useForRhythmPartAddress = 0x15;
    if (sysex.size() == 11 && sysex[0] == 0xf0 && sysex[1] == 0x41
        && sysex[2] <= 0x1f && sysex[3] == 0x42 && sysex[4] == 0x12
        && sysex[rolandAddressHigh] == 0x40
        && sysex[rolandAddressLow] == useForRhythmPartAddress
        && sysex[rolandData] <= 0x02 && sysex[10] == 0xf7) {
        const auto checksum = static_cast<std::uint8_t>((0x80
            - ((sysex[rolandAddressHigh] + sysex[rolandPartBlock]
                + sysex[rolandAddressLow] + sysex[rolandData]) & 0x7f))
            & 0x7f);
        const auto part = rolandGsPartIndex(sysex[rolandPartBlock]);
        if (part && sysex[rolandChecksum] == checksum) {
            const auto map = sysex[rolandData];
            const bool changed = rhythmMaps[*part] != map;
            explicitPartModes[*part] = true;
            if (!changed)
                return std::nullopt;
            rhythmMaps[*part] = map;
            return XgPartModeChange { *part, map != 0 };
        }
    }

    constexpr std::size_t partOffset = 5;
    constexpr std::size_t addressOffset = 6;
    constexpr std::size_t dataOffset = 7;
    constexpr std::uint8_t partParameterGroup = 0x08;
    constexpr std::uint8_t partModeAddress = 0x07;

    if (sysex.size() <= dataOffset + 1 || sysex.front() != 0xf0
        || sysex.back() != 0xf7 || sysex[1] != 0x43
        || (sysex[2] & 0xf0) != 0x10 || sysex[3] != 0x4c
        || sysex[4] != partParameterGroup || sysex[partOffset] >= partCount) {
        return std::nullopt;
    }

    const auto part = static_cast<std::size_t>(sysex[partOffset]);
    const auto startAddress = sysex[addressOffset];
    for (std::size_t index = dataOffset; index + 1 < sysex.size(); ++index) {
        const auto address = static_cast<std::size_t>(startAddress)
            + index - dataOffset;
        if (address != partModeAddress)
            continue;
        const bool rhythm = sysex[index] != 0;
        const bool changed = isRhythm(part) != rhythm;
        explicitPartModes[part] = true;
        if (!changed)
            return std::nullopt;
        rhythmMaps[part] = rhythm ? 1 : 0;
        return XgPartModeChange { part, rhythm };
    }
    return std::nullopt;
}

std::optional<XgPartModeChange> XgPartModes::selectBankMsb(
    std::size_t part, std::uint8_t bankMsb) noexcept
{
    if (part >= partCount || explicitPartModes[part])
        return std::nullopt;

    bool rhythm = false;
    if (systemMode == MidiSystemReset::xg) {
        rhythm = bankMsb == rhythmBankMsb;
    } else if (systemMode == MidiSystemReset::gm2) {
        if (bankMsb == gm2RhythmBankMsb)
            rhythm = true;
        else if (bankMsb != gm2MelodicBankMsb)
            return std::nullopt;
    } else {
        return std::nullopt;
    }
    if (isRhythm(part) == rhythm)
        return std::nullopt;
    rhythmMaps[part] = rhythm ? 1 : 0;
    return XgPartModeChange { part, rhythm };
}

bool XgPartModes::isRhythm(std::size_t part) const noexcept
{
    return part < rhythmMaps.size() && rhythmMaps[part] != 0;
}

std::optional<std::uint8_t> XgPartModes::sharedRhythmMap(
    std::size_t part) const noexcept
{
    if (systemMode != MidiSystemReset::gs || part >= rhythmMaps.size()
        || rhythmMaps[part] == 0) {
        return std::nullopt;
    }
    return rhythmMaps[part];
}

bool XgPartModes::sharesRhythmMap(
    std::size_t first, std::size_t second) const noexcept
{
    const auto map = sharedRhythmMap(first);
    return map && sharedRhythmMap(second) == map;
}

std::uint8_t XgPartModes::effectiveBankMsb(
    std::size_t part, std::uint8_t selectedBankMsb) const noexcept
{
    if (!isRhythm(part))
        return selectedBankMsb;
    if (systemMode == MidiSystemReset::xg)
        return rhythmBankMsb;
    return gm2RhythmBankMsb;
}

std::uint8_t XgPartModes::effectiveBankLsb(
    std::size_t part, std::uint8_t selectedBankLsb) const noexcept
{
    if (!isRhythm(part))
        return selectedBankLsb;
    return 0;
}

} // namespace hybrid
