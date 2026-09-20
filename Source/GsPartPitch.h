#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace hybrid {

struct GsPartKeyShift {
    std::size_t part {};
    std::uint8_t rpnCoarseTune {};
};

[[nodiscard]] std::optional<GsPartKeyShift> gsPartKeyShift(
    std::span<const std::uint8_t> sysex) noexcept;

} // namespace hybrid
