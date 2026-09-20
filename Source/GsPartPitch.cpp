#include "GsPartPitch.h"

namespace hybrid {

namespace {

constexpr std::uint8_t rolandManufacturer = 0x41;
constexpr std::uint8_t gsModel = 0x42;
constexpr std::uint8_t dataSetCommand = 0x12;
constexpr std::uint8_t partParameterGroup = 0x40;
constexpr std::uint8_t pitchKeyShiftAddress = 0x16;
constexpr std::uint8_t minimumKeyShift = 0x28;
constexpr std::uint8_t maximumKeyShift = 0x58;

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

bool hasValidRolandChecksum(std::span<const std::uint8_t> sysex) noexcept
{
    std::uint32_t sum = 0;
    for (std::size_t index = 5; index + 2 < sysex.size(); ++index)
        sum += sysex[index];
    const auto expected = static_cast<std::uint8_t>(
        (0x80 - (sum & 0x7f)) & 0x7f);
    return sysex[sysex.size() - 2] == expected;
}

} // namespace

std::optional<GsPartKeyShift> gsPartKeyShift(
    std::span<const std::uint8_t> sysex) noexcept
{
    constexpr std::size_t minimumMessageSize = 11;
    constexpr std::size_t partBlockIndex = 6;
    constexpr std::size_t startAddressIndex = 7;
    constexpr std::size_t dataIndex = 8;

    if (sysex.size() < minimumMessageSize || sysex.front() != 0xf0
        || sysex.back() != 0xf7 || sysex[1] != rolandManufacturer
        || sysex[2] > 0x1f || sysex[3] != gsModel
        || sysex[4] != dataSetCommand || sysex[5] != partParameterGroup
        || !hasValidRolandChecksum(sysex)) {
        return std::nullopt;
    }

    const auto part = rolandGsPartIndex(sysex[partBlockIndex]);
    const auto startAddress = sysex[startAddressIndex];
    if (!part || startAddress > pitchKeyShiftAddress)
        return std::nullopt;

    const auto offset = static_cast<std::size_t>(
        pitchKeyShiftAddress - startAddress);
    const auto dataSize = sysex.size() - dataIndex - 2;
    if (offset >= dataSize)
        return std::nullopt;

    const auto keyShift = sysex[dataIndex + offset];
    if (keyShift < minimumKeyShift || keyShift > maximumKeyShift)
        return std::nullopt;
    return GsPartKeyShift { *part, keyShift };
}

} // namespace hybrid
