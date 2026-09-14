#pragma once

#include <cstdint>
#include <span>

namespace hybrid {

enum class MuVoiceMapMode : std::uint8_t {
    basic = 0x00,
    native = 0x01,
};

class MuVoiceMapSelector {
public:
    // Returns true only for a valid MU Voice Map Select parameter change.
    bool observe(std::span<const std::uint8_t> sysex) noexcept;

    [[nodiscard]] MuVoiceMapMode mode() const noexcept;
    [[nodiscard]] bool allows2006Voice(std::uint8_t bankMsb,
                                       std::uint8_t bankLsb) const noexcept;

private:
    MuVoiceMapMode currentMode { MuVoiceMapMode::native };
};

} // namespace hybrid
