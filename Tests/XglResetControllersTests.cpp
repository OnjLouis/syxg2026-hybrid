#include "XglResetControllers.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace {

void expect(bool condition, const char* message)
{
    if (condition)
        return;
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
}

std::uint8_t resetValue(std::uint8_t controller)
{
    for (const auto& reset : hybrid::privateXglControllerResets) {
        if (reset.controller == controller)
            return reset.value;
    }
    expect(false, "required controller reset is missing");
    return 0;
}

} // namespace

int main()
{
    constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 21> expected {{
        {1, 0}, {5, 0}, {7, 100}, {10, 64}, {11, 127},
        {64, 0}, {65, 0}, {66, 0}, {67, 0},
        {71, 64}, {72, 64}, {73, 64}, {74, 64}, {75, 64}, {76, 64},
        {77, 64}, {78, 64}, {84, 0}, {91, 0}, {93, 0}, {94, 0},
    }};
    for (const auto [controller, value] : expected)
        expect(resetValue(controller) == value,
               "controller has the required reset value");

    std::array<bool, 128> seen {};
    for (const auto& reset : hybrid::privateXglControllerResets) {
        expect(!seen[reset.controller], "controller reset is not duplicated");
        seen[reset.controller] = true;
    }
}
