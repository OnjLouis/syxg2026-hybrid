#include "XgVariationRouting.h"

namespace hybrid {
namespace {

constexpr std::uint8_t variationConnectionAddress = 0x5a;
constexpr std::uint8_t variationPartAddress = 0x5b;
constexpr std::uint8_t variationPartOff = 0x7f;
constexpr std::size_t midiPartCount = 16;

} // namespace

void XgVariationRouting::reset() noexcept
{
    currentConnection = XgVariationConnection::insertion;
    currentPart.reset();
}

void XgVariationRouting::observe(
    std::span<const std::uint8_t> sysex) noexcept
{
    constexpr std::size_t dataOffset = 7;
    if (sysex.size() <= dataOffset + 1 || sysex.front() != 0xf0
        || sysex.back() != 0xf7 || sysex[1] != 0x43
        || (sysex[2] & 0xf0) != 0x10 || sysex[3] != 0x4c
        || sysex[4] != 0x02 || sysex[5] != 0x01) {
        return;
    }

    const auto startAddress = sysex[6];
    for (std::size_t index = dataOffset; index + 1 < sysex.size(); ++index) {
        const auto address = static_cast<std::size_t>(startAddress)
            + index - dataOffset;
        const auto value = sysex[index];
        if (address == variationConnectionAddress && value <= 1) {
            currentConnection = value == 0
                ? XgVariationConnection::insertion
                : XgVariationConnection::system;
        } else if (address == variationPartAddress) {
            if (value < midiPartCount)
                currentPart = value;
            else if (value == variationPartOff)
                currentPart.reset();
        }
    }
}

XgVariationConnection XgVariationRouting::connection() const noexcept
{
    return currentConnection;
}

std::optional<std::size_t> XgVariationRouting::assignedPart() const noexcept
{
    return currentPart;
}

bool XgVariationRouting::isInsertionPart(std::size_t part) const noexcept
{
    return currentConnection == XgVariationConnection::insertion
        && currentPart == part;
}

} // namespace hybrid
