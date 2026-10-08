#pragma once
#include <atomic>
#include <cstdint>
#include <span>

namespace hybrid {
enum class VoiceSource : std::uint8_t { automatic, keyboard2006, mu };
class VoiceSourcePreference {
public:
    VoiceSource get() const noexcept { return static_cast<VoiceSource>(value.load()); }
    const wchar_t* label() const noexcept {
        switch (get()) {
        case VoiceSource::keyboard2006: return L"2006LE first";
        case VoiceSource::mu: return L"MU first";
        default: return L"Automatic";
        }
    }
    void set(VoiceSource source) noexcept { value.store(static_cast<std::uint8_t>(source)); }
    bool observe(std::span<const std::uint8_t> bytes) noexcept {
        // Private experimental protocol, not a Yamaha hardware command.
        if (bytes.size() != 8 || bytes[0] != 0xf0 || bytes[1] != 0x7d
            || bytes[2] != 'S' || bytes[3] != 'H' || bytes[4] != 'M'
            || bytes[5] != 1 || bytes[6] > 2 || bytes[7] != 0xf7)
            return false;
        set(static_cast<VoiceSource>(bytes[6]));
        return true;
    }
    bool use2006(bool keyboardAvailable, bool muAvailable, bool automaticChoice) const noexcept {
        switch (get()) {
        case VoiceSource::keyboard2006: return keyboardAvailable;
        case VoiceSource::mu: return !muAvailable && keyboardAvailable;
        default: return automaticChoice;
        }
    }
private:
    std::atomic<std::uint8_t> value {};
};
} // namespace hybrid
