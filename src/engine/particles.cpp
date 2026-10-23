#include "particles.h"
#include "scene.h"

namespace px {

static uint32_t lerpColor(uint32_t a, uint32_t b, double t) {
  auto ch = [&](int sh) {
    double ca = (a >> sh) & 255, cb = (b >> sh) & 255;
    return (uint32_t)(ca + (cb - ca) * t) & 255;
  };
  return (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

ParticleEmitter::ParticleEmitter(Scene* s, Texture* t, const EmitterConfig& c)
    : GameObject(s), cfg(c), tex(t), particleX(c.x), particleY(c.y) {
  emitting = c.emitting;
  frequency_ = c.frequency;
  blend = c.blend;
  frame_ = t->get(c.frame);
  if (!frame_) frame_ = t->base();
}

void ParticleEmitter::emitParticle(int count, bool hasXY, double ex, double ey) {
  auto atLimit = [&]() {
    return cfg.maxParticles > 0 && (int)alive.size() + dead_ >= cfg.maxParticles;
  };
  if (atLimit()) return;
  for (int i = 0; i < count; i++) {
    if (dead_ > 0) dead_--;
    Particle p;
    p.frame = frame_;
    if (cfg.zone) cfg.zone(p.x, p.y);
    p.x += hasXY ? ex : particleX.get();
    p.y += hasXY ? ey : particleY.get();
    double speed = cfg.speed.get();
    double ang = rad(cfg.angle.get());
    p.vx = std::cos(ang) * speed;
    p.vy = std::sin(ang) * speed;
    p.life = p.lifeCurrent = cfg.lifespan.get();
    p.scaleStart = cfg.scaleStart; p.scaleEnd = cfg.scaleEnd;
    p.alphaStart = cfg.alphaStart; p.alphaEnd = cfg.alphaEnd;
    p.tintStart = cfg.tintStart; p.tintEnd = cfg.tintEnd;
    p.scale = p.scaleStart; p.alpha = p.alphaStart; p.tint = p.tintStart;
    if (cfg.emitCallback) cfg.emitCallback(p);
    alive.push_back(p);
    if (cfg.stopAfter > 0) {
      stopCounter_++;
      if (stopCounter_ >= cfg.stopAfter) break;
    }
    if (atLimit()) break;
  }
}

void ParticleEmitter::explode(int count, double x, double y) {
  frequency_ = -1;
  emitParticle(count, true, x, y);
}

void ParticleEmitter::update(double deltaMs) {
  double d = deltaMs * timeScale;
  double step = d / 1000.0;
  for (size_t i = 0; i < alive.size();) {
    Particle& p = alive[i];
    if (p.lifeCurrent <= 0) {  // already dead from previous step
      alive.erase(alive.begin() + i);
      dead_++;
      continue;
    }
    double t = p.life > 0 ? 1 - p.lifeCurrent / p.life : 1;
    p.vx += cfg.gravityX * step;
    p.vy += cfg.gravityY * step;
    p.x += p.vx * step;
    p.y += p.vy * step;
    p.scale = p.scaleStart + (p.scaleEnd - p.scaleStart) * t;
    p.alpha = clamp(p.alphaStart + (p.alphaEnd - p.alphaStart) * t, 0, 1);
    p.tint = lerpColor(p.tintStart, p.tintEnd, t);
    p.lifeCurrent -= d;
    if (p.lifeCurrent <= 0) {
      alive.erase(alive.begin() + i);
      dead_++;
    } else i++;
  }
  if (emitting) {
    if (frequency_ == 0) {
      emitParticle(cfg.quantity, false, 0, 0);
    } else if (frequency_ > 0) {
      flowCounter_ -= d;
      while (flowCounter_ <= 0) {
        emitParticle(cfg.quantity, false, 0, 0);
        flowCounter_ += frequency_;
      }
    }
    if (cfg.stopAfter > 0 && stopCounter_ >= cfg.stopAfter) stop();
  }
}

void ParticleEmitter::render(Renderer& r, const Mat& world, double a) {
  for (const Particle& p : alive) {
    if (!p.frame) continue;
    Mat pm = world * Mat::TRS(p.x, p.y, 0, p.scale, p.scale);
    drawFrameQuad(r, pm, p.frame, 0.5, 0.5, false, false, p.tint, p.alpha * a, blend);
  }
}

}  // namespace px
