#pragma once

#include "NativeVlProtocol.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace hybrid {

class NativeVlClient {
public:
    static constexpr std::uint32_t maxFrames = ipc::maxFrames;
    static constexpr std::size_t planeCount = ipc::planeCount;

    NativeVlClient(const std::filesystem::path& workerPath,
                   const std::filesystem::path& vxdPath,
                   std::uint32_t sampleRate);
    ~NativeVlClient();

    NativeVlClient(const NativeVlClient&) = delete;
    NativeVlClient& operator=(const NativeVlClient&) = delete;

    void sendShort(std::uint32_t packedMessage);
    void sendSysex(std::span<const std::uint8_t> bytes);
    void setSampleRate(std::uint32_t sampleRate);
    void warmUp();
    void prepare();
    void render(std::uint32_t frames);
    void beginTimedRender(std::uint32_t frames,
                          std::span<const ipc::TimedMidiEvent> events);
    void finishTimedRender();
    [[nodiscard]] std::span<const std::int16_t> plane(
        std::size_t index, std::uint32_t frames) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace hybrid
