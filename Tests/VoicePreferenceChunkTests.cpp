#include "VoicePreferenceChunk.h"
#include <cstdio>
int main() {
    int failures = 0;
    const auto expect = [&](bool ok) { if (!ok) ++failures; };
    hybrid::VoicePreferenceChunk chunks;
    const std::array<std::uint8_t, 4> original {1,2,3,4};
    hybrid::VoiceSource preference;
    std::span<const std::uint8_t> child;
    for (const auto mode : {hybrid::VoiceSource::automatic, hybrid::VoiceSource::keyboard2006, hybrid::VoiceSource::mu}) {
        auto saved = chunks.save(original, mode);
        expect(hybrid::VoicePreferenceChunk::load(saved, preference, child));
        expect(preference == mode && std::equal(child.begin(), child.end(), original.begin(), original.end()));
        saved[8] = 3; expect(!hybrid::VoicePreferenceChunk::load(saved, preference, child));
        saved[8] = 0; saved[12] = 255; expect(!hybrid::VoicePreferenceChunk::load(saved, preference, child));
        expect(!hybrid::VoicePreferenceChunk::load(std::span(saved).first(10), preference, child));
    }
    expect(hybrid::VoicePreferenceChunk::load(original, preference, child));
    expect(preference == hybrid::VoiceSource::automatic && child.size() == original.size());
    if (failures) std::fprintf(stderr, "%d chunk tests failed\n", failures);
    return failures ? 1 : 0;
}
