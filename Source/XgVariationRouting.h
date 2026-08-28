#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace hybrid {

enum class XgVariationConnection : std::uint8_t {
    insertion,
    system,
};

class XgVariationRouting {
public:
    void reset() noexcept;
    void observe(std::span<const std::uint8_t> sysex) noexcept;

    XgVariationConnection connection() const noexcept;
    std::optional<std::size_t> assignedPart() const noexcept;
    bool isInsertionPart(std::size_t part) const noexcept;

private:
    XgVariationConnection currentConnection {
        XgVariationConnection::insertion
    };
    std::optional<std::size_t> currentPart;
};

} // namespace hybrid
