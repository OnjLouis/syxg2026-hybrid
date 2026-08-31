#include "GsEffectTranslator.h"

#include <array>
#include <cstdio>
#include <initializer_list>

namespace {

int failures {};

void expect(bool condition, const char* message)
{
    if (condition)
        return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    ++failures;
}

std::array<std::uint8_t, 12> effectTypeMessage(
    std::uint8_t msb, std::uint8_t lsb)
{
    std::array<std::uint8_t, 12> message {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40,
        0x03, 0x00, msb, lsb, 0x00, 0xf7,
    };
    const auto sum = 0x40u + 0x03u + msb + lsb;
    message[10] = static_cast<std::uint8_t>((0x80 - (sum & 0x7f)) & 0x7f);
    return message;
}

std::array<std::uint8_t, 11> parameterMessage(
    std::uint8_t addressHigh, std::uint8_t addressMiddle,
    std::uint8_t addressLow, std::uint8_t value)
{
    std::array<std::uint8_t, 11> message {
        0xf0, 0x41, 0x10, 0x42, 0x12, addressHigh,
        addressMiddle, addressLow, value, 0x00, 0xf7,
    };
    const auto sum = static_cast<unsigned>(addressHigh) + addressMiddle
        + addressLow + value;
    message[9] = static_cast<std::uint8_t>((0x80 - (sum & 0x7f)) & 0x7f);
    return message;
}

bool hasParameter(const hybrid::GsEffectChange& change,
                  std::uint8_t address, std::uint8_t first,
                  std::initializer_list<std::uint8_t> second = {})
{
    for (std::size_t index = 0; index < change.parameterCount; ++index) {
        const auto& parameter = change.parameters[index];
        if (parameter.address != address || parameter.firstData != first)
            continue;
        if (second.size() == 0 && !parameter.secondData)
            return true;
        if (second.size() == 1 && parameter.secondData
            && *parameter.secondData == *second.begin()) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    hybrid::GsEffectTranslator translator;
    constexpr std::array<std::uint8_t, 12> distortion {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40,
        0x03, 0x00, 0x01, 0x11, 0x2b, 0xf7,
    };
    const auto distortionChange = translator.observe(distortion);
    expect(distortionChange && distortionChange->type
               && distortionChange->type->msb == 0x49
               && distortionChange->type->lsb == 0,
           "Roland Distortion maps to Yamaha XG Distortion");

    struct MappingCase {
        std::uint8_t gsMsb;
        std::uint8_t gsLsb;
        std::uint8_t xgMsb;
        std::uint8_t xgLsb;
    };
    constexpr std::array mappings {
        MappingCase {0x01, 0x00, 0x4d, 0x00},
        MappingCase {0x01, 0x02, 0x51, 0x00},
        MappingCase {0x01, 0x10, 0x4a, 0x00},
        MappingCase {0x01, 0x20, 0x48, 0x00},
        MappingCase {0x01, 0x22, 0x45, 0x00},
        MappingCase {0x01, 0x30, 0x53, 0x00},
        MappingCase {0x01, 0x42, 0x41, 0x00},
        MappingCase {0x01, 0x72, 0x5e, 0x00},
        MappingCase {0x01, 0x73, 0x5e, 0x00},
        MappingCase {0x11, 0x03, 0x4a, 0x01},
    };
    for (const auto& mapping : mappings) {
        const auto change = translator.observe(
            effectTypeMessage(mapping.gsMsb, mapping.gsLsb));
        expect(change && change->type
                   && change->type->msb == mapping.xgMsb
                   && change->type->lsb == mapping.xgLsb,
               "documented Roland effect maps to its named Yamaha counterpart");
    }

    constexpr std::array<std::uint8_t, 12> unsupportedCompound {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40,
        0x03, 0x00, 0x04, 0x06, 0x33, 0xf7,
    };
    const auto unsupportedChange = translator.observe(unsupportedCompound);
    expect(unsupportedChange && unsupportedChange->type
               && unsupportedChange->type->msb == 0
               && unsupportedChange->type->lsb == 0,
           "unsupported compound effects map to Thru, not a guessed effect");

    const auto guitarMulti = translator.observe(effectTypeMessage(0x04, 0x02));
    expect(guitarMulti && guitarMulti->type
               && guitarMulti->type->msb == 0x49
               && guitarMulti->type->lsb == 0x00,
           "GTR Multi 3 starts on the proven Yamaha Distortion path");

    const auto overdriveSelector = translator.observe(
        parameterMessage(0x40, 0x03, 0x07, 0x00));
    expect(overdriveSelector && overdriveSelector->type
               && overdriveSelector->type->msb == 0x4a
               && overdriveSelector->type->lsb == 0x00,
           "GTR Multi 3 overdrive selector chooses Yamaha Overdrive");

    const auto distortionSelector = translator.observe(
        parameterMessage(0x40, 0x03, 0x07, 0x01));
    expect(distortionSelector && distortionSelector->type
               && distortionSelector->type->msb == 0x49
               && distortionSelector->type->lsb == 0x00,
           "GTR Multi 3 distortion selector chooses Yamaha Distortion");

    const auto drive = translator.observe(
        parameterMessage(0x40, 0x03, 0x08, 0x50));
    expect(drive && hasParameter(*drive, 0x42, 0x00, {0x50}),
           "GTR Multi 3 drive maps to Distortion parameter 1");

    const auto driveOff = translator.observe(
        parameterMessage(0x40, 0x03, 0x0d, 0x00));
    expect(driveOff && hasParameter(*driveOff, 0x42, 0x00, {0x00}),
           "GTR Multi 3 drive switch can bypass distortion");

    const auto driveOn = translator.observe(
        parameterMessage(0x40, 0x03, 0x0d, 0x01));
    expect(driveOn && hasParameter(*driveOn, 0x42, 0x00, {0x50}),
           "GTR Multi 3 drive switch restores the last drive value");

    const auto delayMix = translator.observe(
        parameterMessage(0x40, 0x03, 0x15, 0x64));
    expect(!delayMix,
           "GTR Multi 3 delay mix cannot corrupt a distortion parameter");

    const auto chorusMix = translator.observe(
        parameterMessage(0x40, 0x03, 0x12, 0x64));
    expect(chorusMix && hasParameter(*chorusMix, 0x59, 0x64),
           "GTR Multi 3 chorus mix feeds the Yamaha chorus bus");

    translator.reset();
    (void)translator.observe(effectTypeMessage(0x01, 0x11));
    const auto directDistortionDrive = translator.observe(
        parameterMessage(0x40, 0x03, 0x03, 0x7f));
    expect(directDistortionDrive
               && hasParameter(*directDistortionDrive, 0x42, 0x00, {0x7f}),
           "GS Distortion drive maps to Yamaha Distortion parameter 1");

    translator.reset();
    struct DelayMacroCase {
        std::uint8_t macro;
        std::uint8_t xgMsb;
    };
    constexpr std::array delayMacros {
        DelayMacroCase {0, 0x05}, DelayMacroCase {1, 0x05},
        DelayMacroCase {2, 0x05}, DelayMacroCase {3, 0x05},
        DelayMacroCase {4, 0x08}, DelayMacroCase {5, 0x08},
        DelayMacroCase {6, 0x08}, DelayMacroCase {7, 0x08},
        DelayMacroCase {8, 0x05}, DelayMacroCase {9, 0x07},
    };
    for (const auto& delayMacro : delayMacros) {
        const auto change = translator.observe(
            parameterMessage(0x40, 0x01, 0x50, delayMacro.macro));
        expect(change && change->type
                   && change->type->msb == delayMacro.xgMsb
                   && change->type->lsb == 0,
               "each documented GS delay macro selects an XG delay family");
        expect(change && hasParameter(*change, 0x58,
                                      delayMacro.macro == 8 ? 127 : 0),
               "GS Delay-to-Reverb macro controls the Yamaha reverb send");
    }

    translator.reset();
    (void)translator.observe(parameterMessage(0x40, 0x01, 0x50, 0x00));
    const auto delay500ms = translator.observe(
        parameterMessage(0x40, 0x01, 0x52, 0x69));
    expect(delay500ms && hasParameter(*delay500ms, 0x46, 0x27, {0x08}),
           "GS 500 ms delay converts to Yamaha 0.1 ms word units");

    const auto feedback = translator.observe(
        parameterMessage(0x40, 0x01, 0x59, 0x50));
    expect(feedback && hasParameter(*feedback, 0x4a, 0x00, {0x50}),
           "GS conventional-delay feedback reaches Yamaha parameter 5");

    const auto delayReverbSend = translator.observe(
        parameterMessage(0x40, 0x01, 0x5a, 0x65));
    expect(delayReverbSend && hasParameter(*delayReverbSend, 0x58, 0x65),
           "GS delay-to-reverb send reaches Yamaha variation routing");

    translator.reset();
    (void)translator.observe(parameterMessage(0x40, 0x01, 0x50, 0x04));
    const auto crossDelayTime = translator.observe(
        parameterMessage(0x40, 0x01, 0x52, 0x69));
    expect(crossDelayTime
               && hasParameter(*crossDelayTime, 0x42, 0x27, {0x08})
               && hasParameter(*crossDelayTime, 0x44, 0x27, {0x08}),
           "GS pan-delay time reaches both Yamaha cross-delay paths");

    translator.reset();
    (void)translator.observe(parameterMessage(0x40, 0x01, 0x50, 0x09));
    const auto echoFeedback = translator.observe(
        parameterMessage(0x40, 0x01, 0x59, 0x40));
    expect(echoFeedback
               && hasParameter(*echoFeedback, 0x44, 0x00, {0x40})
               && hasParameter(*echoFeedback, 0x48, 0x00, {0x40}),
           "GS Pan Repeat feedback reaches both Yamaha echo paths");

    constexpr std::array<std::uint8_t, 11> part9On {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40,
        0x49, 0x22, 0x01, 0x54, 0xf7,
    };
    const auto part9Change = translator.observe(part9On);
    expect(part9Change && part9Change->part == 8 && part9Change->enabled,
           "Roland Part 9 EFX On maps to MIDI channel 9");

    constexpr std::array<std::uint8_t, 11> part10Off {
        0xf0, 0x41, 0x10, 0x42, 0x12, 0x40,
        0x40, 0x22, 0x00, 0x5e, 0xf7,
    };
    const auto part10Change = translator.observe(part10Off);
    expect(part10Change && part10Change->part == 9 && !part10Change->enabled,
           "Roland Part 10 EFX Off maps to MIDI channel 10");

    auto badChecksum = distortion;
    badChecksum[10] = 0;
    expect(!translator.observe(badChecksum), "bad Roland checksum is rejected");
    return failures == 0 ? 0 : 1;
}
