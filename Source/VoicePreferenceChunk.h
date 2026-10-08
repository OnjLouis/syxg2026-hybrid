#pragma once
#include "VoiceSourcePreference.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace hybrid {
class VoicePreferenceChunk {
public:
    static constexpr std::size_t headerSize = 16;
    const std::vector<std::uint8_t>& save(std::span<const std::uint8_t> child, VoiceSource source) {
        bytes.assign(headerSize + child.size(), 0);
        std::copy(signature.begin(), signature.end(), bytes.begin());
        bytes[8] = static_cast<std::uint8_t>(source);
        const auto size = static_cast<std::uint32_t>(child.size());
        for (unsigned i = 0; i < 4; ++i) bytes[12 + i] = (size >> (i * 8)) & 255;
        std::copy(child.begin(), child.end(), bytes.begin() + headerSize);
        return bytes;
    }
    static bool tagged(std::span<const std::uint8_t> input) noexcept {
        return input.size() >= signature.size()
            && std::equal(signature.begin(), signature.end(), input.begin());
    }
    static bool load(std::span<const std::uint8_t> input, VoiceSource& source,
                     std::span<const std::uint8_t>& child) noexcept {
        if (!tagged(input)) {
            source = VoiceSource::automatic; child = input; return true;
        }
        if (input.size() < headerSize || input[8] > 2
            || input[9] || input[10] || input[11]) return false;
        std::uint32_t size = 0;
        for (unsigned i = 0; i < 4; ++i) size |= std::uint32_t(input[12 + i]) << (i * 8);
        if (size != input.size() - headerSize) return false;
        source = static_cast<VoiceSource>(input[8]); child = input.subspan(headerSize);
        return true;
    }
private:
    inline static constexpr std::array<std::uint8_t, 8> signature {'H','Y','M','A','P','0','0','1'};
    std::vector<std::uint8_t> bytes;
};
} // namespace hybrid
