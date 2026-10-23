// Port of win-effects.js
#pragma once
#include "engine/scene.h"

void drawExpandingRing(px::Scene* scene, double x, double y, double r0, double r1,
                       double duration, bool fill = false, bool pulse = false,
                       uint32_t color = 16777215);
void spawnFinishParticles(px::Scene* scene, uint32_t particleTint = 16777215,
                          uint32_t ringColor = 16777215);
