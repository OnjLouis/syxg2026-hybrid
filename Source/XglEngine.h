#pragma once

#include "Vst2Abi.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace hybrid {

class XglEngine {
public:
    static constexpr std::size_t partCount = 16;
    static constexpr std::size_t busCount = 8;

    XglEngine(const std::filesystem::path& enginePath,
              const std::filesystem::path& bankPath,
              vst2::HostCallback host, float sampleRate,
              std::int32_t blockSize);
    ~XglEngine();

    XglEngine(const XglEngine&) = delete;
    XglEngine& operator=(const XglEngine&) = delete;

    void setSampleRate(float sampleRate);
    void setBlockSize(std::int32_t blockSize);
    void reset();

    // Returns true only when a note-on belongs to a voice rendered by 2006LE.
    // Releases are queued here but also reach S-YXG50, preventing stuck notes
    // when overlapping notes span a 2006LE/fallback voice transition.
    bool queueShort(std::uint32_t packedMessage, std::int32_t deltaFrames);
    void observeSysex(std::span<const std::uint8_t> sysex) noexcept;

    void render(std::int32_t frames, std::span<float> buses,
                std::size_t busStride);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace hybrid
