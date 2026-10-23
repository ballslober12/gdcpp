// Port of constants.js
#pragma once
#include <cmath>
#include <cstdint>

inline double screenWidth = 1138;  // Math.round(10240 / 9)
inline double groundYOffset = 1138 / 2.0 - 150;
inline void setScreenWidth(double w) {
  screenWidth = w;
  groundYOffset = w / 2 - 150;
}
constexpr double screenHeight = 640, baseUnit = 60, unusedConst180 = 180;
constexpr double fixedTimeStep = 1.0 / 240, gravityConst = 11.540004, physicsConst09 = 0.9,
                 physicsConst1916 = 1.916398, physicsConst600 = 600;
constexpr uint32_t colorGreenTint = 65280, colorCyanTint = 65535;
constexpr double yFlipBase = 460;
inline double flipY(double y) { return yFlipBase - y; }

// colour-channel ids used by ColorManager (player.js tail)
constexpr int num30 = 1000, num31 = 1001;

// JS Math.round (round half up)
inline double jsRound(double x) { return std::floor(x + 0.5); }
