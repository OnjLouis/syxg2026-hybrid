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
    engine.reset();
    preference.set(hybrid::VoiceSource::keyboard2006);
    const std::array<std::uint8_t,9> receiveOff {0xf0,0x43,0x10,0x4c,8,11,0x35,0,0xf7};
    auto receiveOn = receiveOff;
    receiveOn[7] = 1;
    engine.observeSysex(receiveOff,0);
    expect(!engine.queueShort(0x00643c9b,0), "disabled part 12 does not own a 2006 note");
    expect(engine.queueShort(0x00643c90,0), "part 12 mute does not mute part 1");
    engine.observeSysex(receiveOn,0);
    expect(engine.queueShort(0x00643c9b,0), "re-enabled part 12 owns notes again");
    engine.observeSysex(receiveOff,0);
    engine.queueShort(0x00003c8b,0);
    engine.reset();
    expect(engine.queueShort(0x00643c9b,0), "system reset restores note reception");
    auto malformed = receiveOff;
    malformed[8] = 0;
    engine.observeSysex(malformed,0);
    expect(engine.queueShort(0x00643d9b,0), "unterminated SysEx cannot mute a part");
    const std::array<std::uint8_t,10> packedReceiveOff {0xf0,0x43,0x10,0x4c,8,4,0x34,0,0,0xf7};
    engine.observeSysex(packedReceiveOff,0);
    expect(!engine.queueShort(0x00643c94,0), "packed parameter write disables part 5");
    expect(engine.queueShort(0x00643e9b,0), "part 5 mute leaves part 12 enabled");
    return failures ? 1 : 0;
}
