#include "win_effects.h"
#include <memory>
#include "constants.h"

using namespace px;

void drawExpandingRing(Scene* scene, double x, double y, double r0, double r1, double duration,
                       bool fill, bool pulse, uint32_t color) {
  Graphics* g = scene->addGraphics();
  g->setDepth(55)->setBlendMode(Blend::Add);
  auto state = std::make_shared<std::pair<double, double>>(r0, 0.0);  // r, t
  EaseFn e = (fill && !pulse) ? EaseFn(ease::quadOut) : EaseFn(ease::linear);
  scene->tween(
      g, duration, e,
      [=](double p) {
        state->first = r0 + (r1 - r0) * p;
        state->second = p;
        double t = state->second;
        double a = pulse ? (t < 0.5 ? 2 * t : 2 * (1 - t)) : 1 - t;
        g->clear();
        if (fill) {
          g->fillStyle(color, std::max(0.0, a));
          g->fillCircle(x, y, state->first);
        } else {
          g->lineStyle(4, color, std::max(0.0, a));
          g->strokeCircle(x, y, state->first);
        }
      },
      [g]() { g->destroy(); });
}

void spawnFinishParticles(Scene* scene, uint32_t particleTint, uint32_t ringColor) {
  const double n = 200;
  double x = n + (screenWidth - 400) * random01();
  double y = n + 240 * random01();
  drawExpandingRing(scene, x, y, 40, 140 + 60 * random01(), 500, true, true, ringColor);
  EmitterConfig cfg;
  cfg.frame = "square.png";
  cfg.speed = Op::R(520, 920);
  cfg.angle = Op::R(0, 360);
  cfg.scaleStart = 0.4; cfg.scaleEnd = 0.13;
  cfg.alphaStart = 1; cfg.alphaEnd = 0;
  cfg.lifespan = Op::R(0, 500);
  cfg.stopAfter = 25;
  cfg.blend = Blend::Add;
  cfg.tintStart = cfg.tintEnd = particleTint;
  cfg.x = Op::R(-20, 20);
  cfg.y = Op::R(-20, 20);
  ParticleEmitter* e = scene->addParticles(x, y, "GJ_WebSheet", cfg);
  e->setDepth(57);
}
