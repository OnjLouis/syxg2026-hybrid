#include "../Source/Vst2Abi.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <span>

namespace {

constexpr std::int32_t blockSize = 512;
constexpr float sampleRate = 44'100.0f;
constexpr std::size_t eventCount = 128;

vst2::IntPtr host(vst2::AEffect*, std::int32_t opcode, std::int32_t,
                  vst2::IntPtr, void*, float)
{
    switch (opcode) {
    case vst2::hostVersion: return 2400;
    case vst2::hostGetSampleRate: return static_cast<vst2::IntPtr>(sampleRate);
    case vst2::hostGetBlockSize: return blockSize;
    default: return 0;
    }
}

struct EventBatch {
    std::int32_t numEvents {};
    vst2::IntPtr reserved {};
    std::array<vst2::Event*, eventCount> events {};
};

struct Instance {
    HMODULE module {};
    vst2::AEffect* effect {};
};

Instance openInstance(const std::filesystem::path& path)
{
    Instance result;
    result.module = LoadLibraryW(path.c_str());
    if (result.module == nullptr)
        return result;
    const auto entry = reinterpret_cast<vst2::EntryPoint>(
        GetProcAddress(result.module, "main"));
    if (entry == nullptr)
        return result;
    result.effect = entry(host);
    if (result.effect == nullptr || result.effect->magic != vst2::effectMagic)
        return result;
    result.effect->dispatcher(result.effect, vst2::open, 0, 0, nullptr, 0.0f);
    result.effect->dispatcher(result.effect, vst2::setSampleRate, 0, 0, nullptr,
                              sampleRate);
    result.effect->dispatcher(result.effect, vst2::setBlockSize, 0, blockSize,
                              nullptr, 0.0f);
    result.effect->dispatcher(result.effect, vst2::mainsChanged, 0, 1, nullptr,
                              0.0f);
    return result;
}

void closeInstance(Instance& instance)
{
    if (instance.effect != nullptr) {
        instance.effect->dispatcher(instance.effect, vst2::mainsChanged, 0, 0,
                                    nullptr, 0.0f);
        instance.effect->dispatcher(instance.effect, vst2::close, 0, 0, nullptr,
                                    0.0f);
    }
    if (instance.module != nullptr)
        FreeLibrary(instance.module);
}

void setMessage(vst2::MidiEvent& event, std::uint8_t status,
                std::uint8_t data1, std::uint8_t data2)
{
    event.midiData[0] = static_cast<char>(status);
    event.midiData[1] = static_cast<char>(data1);
    event.midiData[2] = static_cast<char>(data2);
}

std::uint64_t render(Instance& instance, EventBatch& batch)
{
    instance.effect->dispatcher(instance.effect, vst2::processEvents, 0, 0,
                                &batch, 0.0f);
    std::array<float, blockSize> left {};
    std::array<float, blockSize> right {};
    std::array<float*, 2> outputs {left.data(), right.data()};
    std::uint64_t hash = 1469598103934665603ull;
    for (int block = 0; block < 40; ++block) {
        std::fill(left.begin(), left.end(), 0.0f);
        std::fill(right.begin(), right.end(), 0.0f);
        instance.effect->processReplacing(instance.effect, nullptr,
                                          outputs.data(), blockSize);
        for (const auto* channel : outputs) {
            for (const auto sample : std::span(channel, blockSize)) {
                hash ^= std::bit_cast<std::uint32_t>(sample);
                hash *= 1099511628211ull;
            }
        }
    }
    return hash;
}

} // namespace

bool compareCount(const char* wrapperPath, const char* directPath,
                  std::size_t count)
{
    std::array<vst2::MidiEvent, eventCount> midi {};
    EventBatch batch;
    batch.numEvents = static_cast<std::int32_t>(count);
    for (std::size_t index = 0; index < count; ++index) {
        setMessage(midi[index], 0xb3, 74,
                   static_cast<std::uint8_t>(index & 0x7f));
        batch.events[index] = reinterpret_cast<vst2::Event*>(&midi[index]);
    }
    setMessage(midi[0], 0xb3, 0, 0);
    // Bank 0/2 is absent from 2006LE and must remain byte-identical to the
    // S-YXG50 fallback, regardless of event-batch size.
    setMessage(midi[1], 0xb3, 32, 2);
    setMessage(midi[2], 0xc3, 5, 0);
    constexpr std::array<std::uint8_t, 22> controllers {
        1, 5, 7, 10, 11, 64, 65, 71, 72, 73, 74,
        91, 93, 94, 98, 99, 100, 101, 120, 121, 123, 6,
    };
    for (std::size_t index = 0; index < controllers.size(); ++index) {
        setMessage(midi[index + 3], 0xb3, controllers[index],
                   static_cast<std::uint8_t>(32 + index));
    }
    setMessage(midi[25], 0xa3, 60, 48);  // polyphonic pressure
    setMessage(midi[26], 0xd3, 72, 0);   // channel pressure
    setMessage(midi[27], 0xe3, 0, 96);   // pitch bend
    setMessage(midi[count - 2], 0x93, 60, 100);
    setMessage(midi[count - 1], 0x83, 60, 0);
    midi[count - 1].deltaFrames = 400;

    auto wrapper = openInstance(wrapperPath);
    auto direct = openInstance(directPath);
    if (wrapper.effect == nullptr || direct.effect == nullptr) {
        std::fprintf(stderr, "failed to load probe instances\n");
        closeInstance(wrapper);
        closeInstance(direct);
        return false;
    }

    const auto wrapperHash = render(wrapper, batch);
    const auto directHash = render(direct, batch);
    std::printf("events=%zu wrapper=%016llx direct=%016llx equal=%s\n", count,
                static_cast<unsigned long long>(wrapperHash),
                static_cast<unsigned long long>(directHash),
                wrapperHash == directHash ? "yes" : "no");
    closeInstance(wrapper);
    closeInstance(direct);
    return wrapperHash == directHash;
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr,
                     "usage: HybridXgForwardingProbe <wrapper.dll> <xg.dll>\n");
        return 2;
    }
    bool passed = true;
    for (const auto count : { 32u, 64u, 65u, 128u })
        passed &= compareCount(argv[1], argv[2], count);
    return passed ? 0 : 1;
}
