#include "GsEffectTranslator.h"

#include <algorithm>
#include <array>

namespace hybrid {
namespace {

constexpr std::uint8_t guitarMulti3Msb = 0x04;
constexpr std::uint8_t guitarMulti3Lsb = 0x02;
constexpr std::uint8_t distortionMsb = 0x01;
constexpr std::uint8_t distortionLsb = 0x11;
constexpr std::uint8_t overdriveLsb = 0x10;
constexpr XgEffectType xgDistortion { 0x49, 0x00 };
constexpr XgEffectType xgOverdrive { 0x4a, 0x00 };

struct EffectMapping {
    std::uint8_t gsMsb;
    std::uint8_t gsLsb;
    std::uint8_t xgMsb;
    std::uint8_t xgLsb;
};

// These are direct named counterparts in the Roland SC-88Pro and Yamaha XG
// effect tables. Untranslated compound effects deliberately select Thru.
constexpr std::array effectMappings {
    EffectMapping { 0x01, 0x00, 0x4d, 0x00 }, // Stereo EQ -> 2-band EQ
    EffectMapping { 0x01, 0x02, 0x51, 0x00 }, // Enhancer
    EffectMapping { 0x01, 0x10, 0x4a, 0x00 }, // Overdrive
    EffectMapping { 0x01, 0x11, 0x49, 0x00 }, // Distortion
    EffectMapping { 0x01, 0x20, 0x48, 0x00 }, // Phaser
    EffectMapping { 0x01, 0x21, 0x4e, 0x00 }, // Auto Wah
    EffectMapping { 0x01, 0x22, 0x45, 0x00 }, // Rotary
    EffectMapping { 0x01, 0x23, 0x43, 0x00 }, // Stereo Flanger
    EffectMapping { 0x01, 0x25, 0x46, 0x00 }, // Tremolo
    EffectMapping { 0x01, 0x26, 0x47, 0x00 }, // Auto Pan
    EffectMapping { 0x01, 0x30, 0x53, 0x00 }, // Compressor
    EffectMapping { 0x01, 0x42, 0x41, 0x00 }, // Stereo Chorus
    EffectMapping { 0x01, 0x50, 0x06, 0x00 }, // Stereo Delay
    EffectMapping { 0x01, 0x56, 0x0a, 0x00 }, // Gate Reverb
    EffectMapping { 0x01, 0x72, 0x5e, 0x00 }, // Lo-Fi 1
    EffectMapping { 0x01, 0x73, 0x5e, 0x00 }, // Lo-Fi 2
    EffectMapping { 0x02, 0x02, 0x5f, 0x01 }, // OD -> Delay
    EffectMapping { 0x02, 0x05, 0x5f, 0x00 }, // Distortion -> Delay
    EffectMapping { 0x11, 0x03, 0x4a, 0x01 }, // Dual OD -> Stereo OD
    EffectMapping { 0x11, 0x04, 0x45, 0x02 }, // OD / Rotary
};

std::optional<std::size_t> rolandPartIndex(std::uint8_t block) noexcept
{
    if (block == 0x40)
        return 9;
    if (block >= 0x41 && block <= 0x49)
        return static_cast<std::size_t>(block - 0x41);
    if (block >= 0x4a && block <= 0x4f)
        return static_cast<std::size_t>(block - 0x40);
    return std::nullopt;
}

bool isValidRolandDt1(std::span<const std::uint8_t> bytes) noexcept
{
    if (bytes.size() < 11 || bytes[0] != 0xf0 || bytes[1] != 0x41
        || bytes[2] > 0x1f || bytes[3] != 0x42 || bytes[4] != 0x12
        || bytes.back() != 0xf7) {
        return false;
    }
    std::uint32_t sum = 0;
    for (std::size_t index = 5; index + 2 < bytes.size(); ++index)
        sum += bytes[index];
    return bytes[bytes.size() - 2]
        == static_cast<std::uint8_t>((0x80 - (sum & 0x7f)) & 0x7f);
}

XgEffectType translateType(std::uint8_t msb, std::uint8_t lsb) noexcept
{
    for (const auto& mapping : effectMappings) {
        if (mapping.gsMsb == msb && mapping.gsLsb == lsb)
            return { mapping.xgMsb, mapping.xgLsb };
    }
    return {}; // Yamaha Thru prevents a stale translated effect persisting.
}

void addByteParameter(GsEffectChange& change, std::uint8_t address,
                      std::uint8_t value) noexcept
{
    if (change.parameterCount == change.parameters.size())
        return;
    change.parameters[change.parameterCount++] = {
        .address = address,
        .firstData = value,
    };
}

void addWordParameter(GsEffectChange& change, std::uint8_t address,
                      std::uint16_t value) noexcept
{
    if (change.parameterCount == change.parameters.size())
        return;
    value = std::min<std::uint16_t>(value, 0x3fff);
    change.parameters[change.parameterCount++] = {
        .address = address,
        .firstData = static_cast<std::uint8_t>((value >> 7) & 0x7f),
        .secondData = static_cast<std::uint8_t>(value & 0x7f),
    };
}

// Roland expresses the system-delay time as a compact stepped index. Yamaha's
// delay word uses 0.1 ms units, packed as two 7-bit bytes.
std::uint16_t rolandDelayTimeUnits(std::uint8_t value) noexcept
{
    value = std::min<std::uint8_t>(value, 0x73);
    if (value <= 0x14)
        return value;
    if (value <= 0x23)
        return static_cast<std::uint16_t>(20 + (value - 0x14) * 2);
    if (value <= 0x2d)
        return static_cast<std::uint16_t>(50 + (value - 0x23) * 5);
    if (value <= 0x37)
        return static_cast<std::uint16_t>(100 + (value - 0x2d) * 10);
    if (value <= 0x46)
        return static_cast<std::uint16_t>(200 + (value - 0x37) * 20);
    if (value <= 0x50)
        return static_cast<std::uint16_t>(500 + (value - 0x46) * 50);
    if (value <= 0x5a)
        return static_cast<std::uint16_t>(1000 + (value - 0x50) * 100);
    if (value <= 0x69)
        return static_cast<std::uint16_t>(2000 + (value - 0x5a) * 200);
    return static_cast<std::uint16_t>(5000 + (value - 0x69) * 500);
}

XgEffectType delayType(std::uint8_t macro) noexcept
{
    if (macro <= 3 || macro == 8)
        return { 0x05, 0x00 }; // Delay L,C,R
    if (macro <= 7)
        return { 0x08, 0x00 }; // Cross Delay
    return { 0x07, 0x00 }; // Echo / Pan Repeat
}

} // namespace

void GsEffectTranslator::reset() noexcept
{
    effectMsb_ = 0;
    effectLsb_ = 0;
    guitarDrive_ = 64;
    delayMacro_ = 0;
    guitarDriveEnabled_ = true;
    insertionEffectActive_ = false;
}

std::optional<GsEffectChange> GsEffectTranslator::observe(
    std::span<const std::uint8_t> sysex) noexcept
{
    if (!isValidRolandDt1(sysex))
        return std::nullopt;

    if (sysex[5] == 0x40 && sysex[6] == 0x03 && sysex[7] == 0x00
        && sysex.size() == 12) {
        effectMsb_ = sysex[8];
        effectLsb_ = sysex[9];
        guitarDrive_ = effectMsb_ == guitarMulti3Msb
                && effectLsb_ == guitarMulti3Lsb
            ? 80
            : effectMsb_ == distortionMsb && effectLsb_ == distortionLsb
            ? 76
            : effectMsb_ == distortionMsb && effectLsb_ == overdriveLsb
            ? 48
            : 64;
        guitarDriveEnabled_ = true;
        if (effectMsb_ == guitarMulti3Msb && effectLsb_ == guitarMulti3Lsb) {
            insertionEffectActive_ = true;
            return GsEffectChange { .type = xgDistortion };
        }
        const auto translated = translateType(effectMsb_, effectLsb_);
        insertionEffectActive_ = translated.msb != 0 || translated.lsb != 0;
        return GsEffectChange { .type = translated };
    }

    if (sysex[5] == 0x40 && sysex[6] == 0x03 && sysex.size() == 11
        && effectMsb_ == distortionMsb
        && (effectLsb_ == distortionLsb || effectLsb_ == overdriveLsb)) {
        GsEffectChange change;
        switch (sysex[7]) {
        case 0x03: // Drive.
            addWordParameter(change, 0x42, sysex[8]);
            break;
        case 0x13: // Low EQ gain.
            addWordParameter(change, 0x46, sysex[8]);
            break;
        case 0x14: // High gain is closest to Yamaha's mid-band gain.
            addWordParameter(change, 0x50, sysex[8]);
            break;
        case 0x15: // Pan.
            addByteParameter(change, 0x57, sysex[8]);
            break;
        case 0x16: // Output level.
            addWordParameter(change, 0x4a, sysex[8]);
            break;
        default:
            return std::nullopt;
        }
        return change;
    }

    if (sysex[5] == 0x40 && sysex[6] == 0x03 && sysex.size() == 11
        && effectMsb_ == guitarMulti3Msb
        && effectLsb_ == guitarMulti3Lsb) {
        const auto parameter = sysex[7];
        const auto value = sysex[8];
        GsEffectChange change;
        switch (parameter) {
        case 0x07: // Overdrive/distortion selector.
            change.type = value == 0 ? xgOverdrive : xgDistortion;
            break;
        case 0x08: // Drive.
            guitarDrive_ = value;
            if (guitarDriveEnabled_)
                addWordParameter(change, 0x42, value);
            break;
        case 0x0b: // Low EQ gain.
            addWordParameter(change, 0x46, value);
            break;
        case 0x0c: // High gain is the closest available mid-band control.
            addWordParameter(change, 0x50, value);
            break;
        case 0x0d: // Distortion switch.
            guitarDriveEnabled_ = value != 0;
            addWordParameter(change, 0x42,
                guitarDriveEnabled_ ? guitarDrive_ : 0);
            break;
        case 0x12: // Internal chorus/flanger mix -> XG chorus send.
            addByteParameter(change, 0x59, value);
            break;
        case 0x16: // Effect level.
            addWordParameter(change, 0x4a, value);
            break;
        default:
            return std::nullopt;
        }
        return change;
    }

    if (sysex[5] == 0x40 && sysex[6] == 0x01 && sysex.size() == 11
        && sysex[7] >= 0x50 && sysex[7] <= 0x5a) {
        const auto parameter = sysex[7];
        const auto value = sysex[8];
        if (parameter == 0x50) {
            if (value > 9)
                return std::nullopt;
            delayMacro_ = value;
            if (insertionEffectActive_)
                return std::nullopt;
            GsEffectChange change { .type = delayType(delayMacro_) };
            addByteParameter(change, 0x58, delayMacro_ == 8 ? 127 : 0);
            return change;
        }
        if (insertionEffectActive_)
            return std::nullopt;

        GsEffectChange change;
        switch (parameter) {
        case 0x52: {
            const auto time = rolandDelayTimeUnits(value);
            if (delayMacro_ <= 3 || delayMacro_ == 8) {
                addWordParameter(change, 0x46, time);
            } else if (delayMacro_ <= 7) {
                addWordParameter(change, 0x42, time);
                addWordParameter(change, 0x44, time);
            } else {
                addWordParameter(change, 0x42, time);
                addWordParameter(change, 0x46, time);
            }
            break;
        }
        case 0x55: // Center level for Delay L,C,R.
            if (delayMacro_ <= 3 || delayMacro_ == 8)
                addWordParameter(change, 0x4c, value);
            break;
        case 0x56: // Left level for Delay L,C,R.
            if (delayMacro_ <= 3 || delayMacro_ == 8)
                addWordParameter(change, 0x4e, value);
            break;
        case 0x57: // Right level for Delay L,C,R.
            if (delayMacro_ <= 3 || delayMacro_ == 8)
                addWordParameter(change, 0x50, value);
            break;
        case 0x59: // Feedback layout differs between XG delay families.
            if (delayMacro_ <= 3 || delayMacro_ == 8) {
                addWordParameter(change, 0x4a, value);
            } else if (delayMacro_ <= 7) {
                addWordParameter(change, 0x46, value);
            } else {
                addWordParameter(change, 0x44, value);
                addWordParameter(change, 0x48, value);
            }
            break;
        case 0x5a:
            addByteParameter(change, 0x58, value);
            break;
        default:
            return std::nullopt;
        }
        return change.parameterCount == 0
            ? std::nullopt
            : std::optional<GsEffectChange> { change };
    }

    if (sysex[5] == 0x40 && sysex[7] == 0x22 && sysex.size() == 11
        && sysex[8] <= 1) {
        if (const auto part = rolandPartIndex(sysex[6])) {
            return GsEffectChange {
                .part = part,
                .enabled = sysex[8] != 0,
            };
        }
    }
    return std::nullopt;
}

} // namespace hybrid
