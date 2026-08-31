#pragma once

#include <array>
#include <cstdint>

namespace hybrid {

struct ControllerReset {
    std::uint8_t controller {};
    std::uint8_t value {};
};

inline constexpr std::array privateXglControllerResets {
    ControllerReset {1, 0},
    ControllerReset {5, 0},
    ControllerReset {7, 100},
    ControllerReset {10, 64},
    ControllerReset {11, 127},
    ControllerReset {64, 0},
    ControllerReset {65, 0},
    ControllerReset {66, 0},
    ControllerReset {67, 0},
    ControllerReset {71, 64},
    ControllerReset {72, 64},
    ControllerReset {73, 64},
    ControllerReset {74, 64},
    ControllerReset {75, 64},
    ControllerReset {76, 64},
    ControllerReset {77, 64},
    ControllerReset {78, 64},
    ControllerReset {84, 0},
    ControllerReset {91, 0},
    ControllerReset {93, 0},
    ControllerReset {94, 0},
};

} // namespace hybrid
