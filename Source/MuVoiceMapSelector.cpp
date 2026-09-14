#include "MuVoiceMapSelector.h"

namespace hybrid {
namespace {

constexpr std::size_t voiceMapMessageSize = 9;
constexpr std::uint8_t yamahaManufacturer = 0x43;
constexpr std::uint8_t parameterChangeDevicePrefix = 0x10;
constexpr std::uint8_t muNativeModel = 0x49;
constexpr std::uint8_t voiceMapAddress = 0x12;

} // namespace

bool MuVoiceMapSelector::observe(
    std::span<const std::uint8_t> sysex) noexcept
{
    if (sysex.size() != voiceMapMessageSize || sysex.front() != 0xf0
        || sysex.back() != 0xf7 || sysex[1] != yamahaManufacturer
        || (sysex[2] & 0xf0) != parameterChangeDevicePrefix
        || sysex[3] != muNativeModel || sysex[4] != 0x00
        || sysex[5] != 0x00 || sysex[6] != voiceMapAddress
        || sysex[7] > static_cast<std::uint8_t>(MuVoiceMapMode::native)) {
        return false;
    }

    currentMode = static_cast<MuVoiceMapMode>(sysex[7]);
    return true;
}

MuVoiceMapMode MuVoiceMapSelector::mode() const noexcept
{
    return currentMode;
}

bool MuVoiceMapSelector::allows2006Voice(std::uint8_t bankMsb,
                                         std::uint8_t bankLsb) const noexcept
{
    return currentMode != MuVoiceMapMode::basic
        || bankMsb != 0 || bankLsb != 0;
}

} // namespace hybrid
