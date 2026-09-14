#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace hybrid {

class XglVoiceMap {
public:
    static constexpr std::size_t bankMapOffset = 0x40;
    static constexpr std::size_t bankMapSize = 128 * 128;
    static constexpr std::size_t compactBankCount = 87;
    static constexpr std::size_t programsPerBank = 128;
    static constexpr std::size_t programMapOffset = bankMapOffset + bankMapSize;
    static constexpr std::size_t programMapSize = compactBankCount
        * programsPerBank * sizeof(std::uint16_t);
    static constexpr std::size_t requiredTableSize = programMapOffset
        + programMapSize;

    explicit XglVoiceMap(std::span<const std::uint8_t> table);
    static XglVoiceMap load(const std::filesystem::path& path);

    [[nodiscard]] bool hasVoice(std::uint8_t bankMsb,
                                std::uint8_t bankLsb,
                                std::uint8_t program) const noexcept;
    [[nodiscard]] bool shouldUse2006Engine(std::uint8_t bankMsb,
                                           std::uint8_t bankLsb,
                                           std::uint8_t program) const noexcept;

private:
    static constexpr std::uint8_t missingBank = 0xff;
    static constexpr std::uint16_t missingVoice = 0xffff;

    std::array<std::uint8_t, bankMapSize> banks {};
    std::array<std::uint16_t, compactBankCount * programsPerBank> voices {};
};

} // namespace hybrid
