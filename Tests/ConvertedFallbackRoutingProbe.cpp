#include "ConvertedMuVoiceMap.h"
#include "VoiceSourcePreference.h"
#include "XglEngine.h"
#include <cstdio>
vst2::IntPtr host(vst2::AEffect*, std::int32_t opcode, std::int32_t, vst2::IntPtr, void*, float) {
    if (opcode == vst2::hostVersion) return 2400;
    if (opcode == vst2::hostGetSampleRate) return 48000;
    if (opcode == vst2::hostGetBlockSize) return 512;
    return 0;
}
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    const auto catalog = hybrid::ConvertedMuVoiceMap::load(argv[1]);
    if (!catalog.valid() || !catalog.hasVoice(48,88,5,false)) return 3;
    hybrid::VoiceSourcePreference preference;
    hybrid::XglEngine engine(argv[2],argv[3],host,48000,512);
    engine.setVoicePreference(&preference);
    engine.setConvertedVoiceMap(&catalog);
    int failures = 0;
    const auto expect = [&](bool ok, const char* label) {
        if (!ok) { std::fprintf(stderr,"FAIL: %s\n",label); ++failures; }
    };
    expect(engine.queueShort(0x00643c90,0), "automatic uses 2006 piano");
    preference.set(hybrid::VoiceSource::mu);
    expect(!engine.queueShort(0x00643d90,0), "MU-first uses converted piano");
    engine.queueShort(0x007020b0,0);
    expect(engine.queueShort(0x00643e90,0), "MU gap falls back to keyboard panel bank");
    preference.set(hybrid::VoiceSource::keyboard2006);
    engine.queueShort(0x003000b0,0); engine.queueShort(0x005820b0,0); engine.queueShort(0x000005c0,0);
    expect(!engine.queueShort(0x00643f90,0), "keyboard gap falls back to Anathema");
    engine.reset();
    preference.set(hybrid::VoiceSource::automatic);
    const std::array<std::uint8_t,9> partMode {0xf0,0x43,0x10,0x4c,8,8,7,3,0xf7};
    engine.observeSysex(partMode,0);
    engine.queueShort(0x007e00b8,0); engine.queueShort(0x0010c8,0);
    expect(!engine.queueShort(0x00642698,0), "SFX Techno kit does not become 2006 Rock kit");
    return failures ? 1 : 0;
}
