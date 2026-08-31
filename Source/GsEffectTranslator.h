#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace hybrid {

struct XgEffectType {
    std::uint8_t msb {};
    std::uint8_t lsb {};
};

struct XgEffectParameter {
    std::uint8_t address {};
    std::uint8_t firstData {};
    std::optional<std::uint8_t> secondData;
};

struct GsEffectChange {
    std::optional<XgEffectType> type;
    std::optional<std::size_t> part;
    bool enabled {};
    std::array<XgEffectParameter, 2> parameters {};
    std::size_t parameterCount {};
};

class GsEffectTranslator {
public:
    void reset() noexcept;

    [[nodiscard]] std::optional<GsEffectChange> observe(
        std::span<const std::uint8_t> sysex) noexcept;

private:
    std::uint8_t effectMsb_ {};
    std::uint8_t effectLsb_ {};
    std::uint8_t guitarDrive_ { 64 };
    std::uint8_t delayMacro_ {};
    bool guitarDriveEnabled_ { true };
    bool insertionEffectActive_ {};
};

} // namespace hybrid
