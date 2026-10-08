#include "VoiceSourcePreference.h"
#include <array>
#include <cstdio>
int main() {
    hybrid::VoiceSourcePreference preference;
    int failures = 0;
    const auto expect = [&](bool ok, const char* label) {
        if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); ++failures; }
    };
    expect(preference.use2006(true, true, true), "automatic preserves 2026 routing");
    expect(!preference.use2006(true, true, false), "automatic preserves MU routing");
    const std::array<std::uint8_t, 8> mu {0xf0,0x7d,'S','H','M',1,2,0xf7};
    expect(preference.observe(mu), "private map selection accepted");
    expect(!preference.use2006(true, true, true), "MU preference overrides keyboard priority");
    expect(preference.use2006(true, false, false), "MU gap falls back to keyboard");
    expect(!preference.use2006(false, false, true), "missing keyboard never selected in MU mode");
    auto keyboard = mu; keyboard[6] = 1;
    expect(preference.observe(keyboard), "keyboard selection accepted");
    expect(preference.use2006(true, true, false), "keyboard takes precedence");
    expect(!preference.use2006(false, true, true), "keyboard gap falls back to MU");
    auto malformed = mu; malformed[5] = 2;
    expect(!preference.observe(malformed), "unsupported protocol rejected");
    expect(preference.get() == hybrid::VoiceSource::keyboard2006, "bad input preserves state");
    malformed = mu; malformed[6] = 3;
    expect(!preference.observe(malformed), "unknown map rejected");
    expect(!preference.observe(std::span(mu).first(7)), "truncation rejected");
    return failures ? 1 : 0;
}
