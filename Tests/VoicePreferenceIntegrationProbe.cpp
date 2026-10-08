#include "Vst2Abi.h"
#include <windows.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>
vst2::IntPtr host(vst2::AEffect*, int opcode, int, vst2::IntPtr, void*, float) {
    if (opcode == vst2::hostVersion) return 2400;
    if (opcode == vst2::hostGetSampleRate) return 48000;
    if (opcode == vst2::hostGetBlockSize) return 512;
    return 0;
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto module = LoadLibraryA(argv[1]);
    if (!module) return 3;
    const auto entry = reinterpret_cast<vst2::EntryPoint>(GetProcAddress(module, "VSTPluginMain"));
    if (!entry) return 4;
    auto* effect = entry(host);
    if (!effect) return 5;
    effect->dispatcher(effect, vst2::open, 0, 0, nullptr, 0);
    const int parameter = effect->numParams - 1;
    int failures = 0;
    const auto expect = [&](bool ok, const char* label) {
        if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); ++failures; }
    };
    expect(effect->getParameter(effect, parameter) == 0, "automatic default");
    expect(effect->dispatcher(effect, 26, parameter, 0, nullptr, 0) == 1, "automatable preference");
    char name[8] {};
    effect->dispatcher(effect, 8, parameter, 0, name, 0);
    expect(std::string(name) == "Mapping", "complete parameter name");
    for (int mode = 0; mode <= 2; ++mode) {
        effect->setParameter(effect, parameter, mode / 2.0f);
        void* data = nullptr;
        const auto size = effect->dispatcher(effect, 23, 0, 0, &data, 0);
        expect(size >= 16 && data != nullptr, "wrapped state exists");
        if (size < 16 || data == nullptr) continue;
        std::vector<unsigned char> saved(static_cast<unsigned char*>(data), static_cast<unsigned char*>(data) + size);
        effect->setParameter(effect, parameter, 0);
        effect->dispatcher(effect, 24, 0, saved.size(), saved.data(), 0);
        expect(effect->getParameter(effect, parameter) == mode / 2.0f, "saved mode round trip");
        saved[8] = 127;
        effect->dispatcher(effect, 24, 0, saved.size(), saved.data(), 0);
        expect(effect->getParameter(effect, parameter) == mode / 2.0f, "malformed state does not change preference");
        effect->dispatcher(effect, 24, 0, -1, saved.data(), 0);
        expect(effect->getParameter(effect, parameter) == mode / 2.0f, "negative state rejected");
    }
    effect->setParameter(effect, parameter, std::numeric_limits<float>::quiet_NaN());
    expect(effect->getParameter(effect, parameter) == 1, "NaN ignored");
    for (int mode = 0; mode <= 2; ++mode) {
        std::array<char, 8> command {char(0xf0), 0x7d, 'S', 'H', 'M', 1, char(mode), char(0xf7)};
        vst2::SysexEvent event {};
        event.dumpBytes = command.size();
        event.sysexDump = command.data();
        vst2::Events batch {};
        batch.numEvents = 1;
        batch.events[0] = reinterpret_cast<vst2::Event*>(&event);
        effect->dispatcher(effect, vst2::processEvents, 0, 0, &batch, 0);
        expect(effect->getParameter(effect, parameter) == mode / 2.0f, "MIDI command changes mode");
        command[5] = 2;
        effect->dispatcher(effect, vst2::processEvents, 0, 0, &batch, 0);
        expect(effect->getParameter(effect, parameter) == mode / 2.0f, "unknown MIDI protocol ignored");
    }
    effect->dispatcher(effect, vst2::close, 0, 0, nullptr, 0);
    FreeLibrary(module);
    std::printf("Preference probe failures=%d\n", failures);
    return failures ? 1 : 0;
}
