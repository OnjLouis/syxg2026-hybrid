#include "XglEngine.h"
#include "XglVoiceMap.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace hybrid {
namespace {

constexpr std::size_t maxEventsPerPart = 4096;
constexpr std::uint8_t defaultReverbSend = 40;

constexpr std::uint8_t statusByte(std::uint32_t message) noexcept
{
    return static_cast<std::uint8_t>(message);
}

constexpr std::uint8_t dataByte1(std::uint32_t message) noexcept
{
    return static_cast<std::uint8_t>(message >> 8);
}

constexpr std::uint8_t dataByte2(std::uint32_t message) noexcept
{
    return static_cast<std::uint8_t>(message >> 16);
}

constexpr std::uint8_t operation(std::uint32_t message) noexcept
{
    return statusByte(message) & 0xf0;
}

constexpr std::uint8_t channel(std::uint32_t message) noexcept
{
    return statusByte(message) & 0x0f;
}

constexpr bool isNoteOn(std::uint32_t message) noexcept
{
    return operation(message) == 0x90 && dataByte2(message) != 0;
}

constexpr bool isNoteOff(std::uint32_t message) noexcept
{
    return operation(message) == 0x80
        || (operation(message) == 0x90 && dataByte2(message) == 0);
}

constexpr std::uint32_t remapToPartZero(std::uint32_t message) noexcept
{
    return (message & 0xffffff00u)
        | static_cast<std::uint8_t>(statusByte(message) & 0xf0);
}

std::runtime_error win32Error(const char* context)
{
    return std::runtime_error(std::string(context) + " (Win32 error "
                              + std::to_string(GetLastError()) + ")");
}

struct EventBatch {
    std::int32_t numEvents {};
    vst2::IntPtr reserved {};
    std::array<vst2::Event*, maxEventsPerPart> events {};
};

struct PartState {
    vst2::AEffect* effect {};
    std::vector<vst2::MidiEvent> pending;
    std::array<std::uint8_t, 128> heldNotes {};
    std::uint8_t bankMsb {};
    std::uint8_t bankLsb {};
    std::uint8_t program {};
    std::uint8_t reverbSend { defaultReverbSend };
    std::uint8_t chorusSend {};
    std::uint8_t variationSend {};
};

} // namespace

class XglEngine::Impl {
public:
    Impl(const std::filesystem::path& enginePath,
         const std::filesystem::path& bankPath,
         vst2::HostCallback host, float initialSampleRate,
         std::int32_t initialBlockSize)
        : voiceMap(XglVoiceMap::load(bankPath))
    {
        module = LoadLibraryW(enginePath.c_str());
        if (module == nullptr)
            throw win32Error("cannot load S-YXG2006LE engine");
        entry = reinterpret_cast<vst2::EntryPoint>(
            GetProcAddress(module, "main"));
        if (entry == nullptr)
            throw std::runtime_error("S-YXG2006LE has no VST main export");

        try {
            for (auto& part : parts) {
                part.pending.reserve(maxEventsPerPart);
                part.effect = entry(host);
                if (part.effect == nullptr
                    || part.effect->magic != vst2::effectMagic) {
                    throw std::runtime_error(
                        "cannot create an S-YXG2006LE part");
                }
                part.effect->dispatcher(part.effect, vst2::open, 0, 0,
                                        nullptr, 0.0f);
                configure(part, initialSampleRate, initialBlockSize);
            }
            sampleRate = initialSampleRate;
            blockSize = initialBlockSize;
            reset();
        } catch (...) {
            close();
            throw;
        }
    }

    ~Impl() { close(); }

    void setSampleRate(float requestedRate)
    {
        if (!(requestedRate > 0.0f))
            return;
        sampleRate = requestedRate;
        for (auto& part : parts) {
            part.effect->dispatcher(part.effect, vst2::setSampleRate, 0, 0,
                                    nullptr, requestedRate);
        }
    }

    void setBlockSize(std::int32_t requestedSize)
    {
        if (requestedSize <= 0)
            return;
        blockSize = requestedSize;
        for (auto& part : parts) {
            part.effect->dispatcher(part.effect, vst2::setBlockSize, 0,
                                    requestedSize, nullptr, 0.0f);
        }
    }

    void reset()
    {
        for (auto& part : parts) {
            part.bankMsb = 0;
            part.bankLsb = 0;
            part.program = 0;
            part.reverbSend = defaultReverbSend;
            part.chorusSend = 0;
            part.variationSend = 0;
            part.heldNotes.fill(0);
            part.pending.clear();
            queue(part, 0x000078b0u, 0);
            queue(part, 0x000079b0u, 0);
            queue(part, 0x00005bb0u, 0);
            queue(part, 0x00005db0u, 0);
            queue(part, 0x00005eb0u, 0);
            queue(part, 0x000000b0u, 0);
            queue(part, 0x000020b0u, 0);
            queue(part, 0x000000c0u, 0);
        }
    }

    bool queueShort(std::uint32_t message, std::int32_t deltaFrames)
    {
        const auto partIndex = channel(message);
        auto& part = parts[partIndex];
        const auto op = operation(message);
        const auto first = dataByte1(message);
        const auto second = dataByte2(message);

        if (op == 0xb0 && first == 0)
            part.bankMsb = second;
        else if (op == 0xb0 && first == 32)
            part.bankLsb = second;
        else if (op == 0xc0)
            part.program = first;

        auto remapped = remapToPartZero(message);
        if (op == 0xb0 && first == 91) {
            part.reverbSend = second;
            remapped &= 0x0000ffffu;
        } else if (op == 0xb0 && first == 93) {
            part.chorusSend = second;
            remapped &= 0x0000ffffu;
        } else if (op == 0xb0 && first == 94) {
            part.variationSend = second;
            remapped &= 0x0000ffffu;
        }

        if (isNoteOn(message)) {
            if (!hasSelectedVoice(part))
                return false;
            if (part.heldNotes[first] != 0xff)
                ++part.heldNotes[first];
            queue(part, remapped, deltaFrames);
            return true;
        }
        if (isNoteOff(message)) {
            if (part.heldNotes[first] == 0)
                return false;
            --part.heldNotes[first];
            queue(part, remapped, deltaFrames);
            return false;
        }
        if (op == 0xb0 && (first == 120 || first == 123))
            part.heldNotes.fill(0);
        queue(part, remapped, deltaFrames);
        return false;
    }

    void render(std::int32_t frames, std::span<float> buses,
                std::size_t busStride)
    {
        if (frames <= 0)
            return;
        if (buses.size() < XglEngine::busCount * busStride
            || static_cast<std::size_t>(frames) > busStride) {
            throw std::runtime_error("S-YXG2006LE output bus is too small");
        }
        left.resize(frames);
        right.resize(frames);
        std::array<float*, 2> outputs { left.data(), right.data() };

        for (auto& part : parts) {
            dispatchPending(part, frames);
            std::fill(left.begin(), left.end(), 0.0f);
            std::fill(right.begin(), right.end(), 0.0f);
            const auto process = part.effect->processReplacing != nullptr
                ? part.effect->processReplacing : part.effect->process;
            process(part.effect, nullptr, outputs.data(), frames);
            mixPart(part, buses, busStride, frames);
            part.pending.clear();
        }
    }

private:
    void configure(PartState& part, float rate, std::int32_t size)
    {
        part.effect->dispatcher(part.effect, vst2::setSampleRate, 0, 0,
                                nullptr, rate);
        part.effect->dispatcher(part.effect, vst2::setBlockSize, 0, size,
                                nullptr, 0.0f);
        part.effect->dispatcher(part.effect, vst2::mainsChanged, 0, 1,
                                nullptr, 0.0f);
    }

    void close() noexcept
    {
        for (auto& part : parts) {
            if (part.effect == nullptr)
                continue;
            part.effect->dispatcher(part.effect, vst2::mainsChanged, 0, 0,
                                    nullptr, 0.0f);
            part.effect->dispatcher(part.effect, vst2::close, 0, 0, nullptr,
                                    0.0f);
            part.effect = nullptr;
        }
        if (module != nullptr)
            FreeLibrary(module);
        module = nullptr;
    }

    bool hasSelectedVoice(const PartState& part) const noexcept
    {
        return voiceMap.hasVoice(part.bankMsb, part.bankLsb, part.program);
    }

    static void queue(PartState& part, std::uint32_t message,
                      std::int32_t deltaFrames)
    {
        if (part.pending.size() == maxEventsPerPart)
            throw std::runtime_error("S-YXG2006LE MIDI queue is full");
        vst2::MidiEvent event;
        event.deltaFrames = std::max(0, deltaFrames);
        event.midiData[0] = static_cast<char>(message);
        event.midiData[1] = static_cast<char>(message >> 8);
        event.midiData[2] = static_cast<char>(message >> 16);
        part.pending.push_back(event);
    }

    static void dispatchPending(PartState& part, std::int32_t frames)
    {
        if (part.pending.empty())
            return;
        EventBatch batch;
        batch.numEvents = static_cast<std::int32_t>(part.pending.size());
        for (std::size_t index = 0; index < part.pending.size(); ++index) {
            part.pending[index].deltaFrames = std::min(
                part.pending[index].deltaFrames, std::max(0, frames - 1));
            batch.events[index] = reinterpret_cast<vst2::Event*>(
                &part.pending[index]);
        }
        part.effect->dispatcher(part.effect, vst2::processEvents, 0, 0,
                                &batch, 0.0f);
    }

    void mixPart(const PartState& part, std::span<float> buses,
                 std::size_t stride, std::int32_t frames) const noexcept
    {
        const auto reverb = part.reverbSend / 127.0f;
        const auto chorus = part.chorusSend / 127.0f;
        const auto variation = part.variationSend / 127.0f;
        for (std::int32_t frame = 0; frame < frames; ++frame) {
            const auto l = left[frame];
            const auto r = right[frame];
            buses[0 * stride + frame] += l;
            buses[1 * stride + frame] += r;
            buses[2 * stride + frame] += l * reverb;
            buses[3 * stride + frame] += r * reverb;
            buses[4 * stride + frame] += l * chorus;
            buses[5 * stride + frame] += r * chorus;
            buses[6 * stride + frame] += l * variation;
            buses[7 * stride + frame] += r * variation;
        }
    }

    HMODULE module {};
    vst2::EntryPoint entry {};
    std::array<PartState, XglEngine::partCount> parts;
    XglVoiceMap voiceMap;
    std::vector<float> left;
    std::vector<float> right;
    float sampleRate {};
    std::int32_t blockSize {};
};

XglEngine::XglEngine(const std::filesystem::path& enginePath,
                     const std::filesystem::path& bankPath,
                     vst2::HostCallback host, float sampleRate,
                     std::int32_t blockSize)
    : impl(std::make_unique<Impl>(enginePath, bankPath, host, sampleRate,
                                  blockSize))
{
}

XglEngine::~XglEngine() = default;

void XglEngine::setSampleRate(float sampleRate)
{
    impl->setSampleRate(sampleRate);
}

void XglEngine::setBlockSize(std::int32_t blockSize)
{
    impl->setBlockSize(blockSize);
}

void XglEngine::reset()
{
    impl->reset();
}

bool XglEngine::queueShort(std::uint32_t packedMessage,
                           std::int32_t deltaFrames)
{
    return impl->queueShort(packedMessage, deltaFrames);
}

void XglEngine::render(std::int32_t frames, std::span<float> buses,
                       std::size_t busStride)
{
    impl->render(frames, buses, busStride);
}

} // namespace hybrid
