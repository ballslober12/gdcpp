// Particle emitter reproducing Phaser 3.60+ ParticleEmitter semantics used by the game.
#pragma once
#include <functional>
#include <vector>
#include "objects.h"

namespace px {

// Emitter op: constant or uniform random range.
struct Op {
  double min = 0, max = 0;
  bool range = false;
  Op() {}
  Op(double v) : min(v), max(v) {}
  static Op R(double a, double b) { Op o; o.min = a; o.max = b; o.range = true; return o; }
  double get() const { return range ? floatBetween(min, max) : min; }
};

struct Particle {
  double x = 0, y = 0, vx = 0, vy = 0;
  double life = 0, lifeCurrent = 0;
  double scale = 1, alpha = 1;
  uint32_t tint = 0xffffff;
  double scaleStart = 1, scaleEnd = 1, alphaStart = 1, alphaEnd = 1;
  uint32_t tintStart = 0xffffff, tintEnd = 0xffffff;
  Frame* frame = nullptr;
};

struct EmitterConfig {
  std::string frame = "square.png";
  Op speed = Op(0), angle = Op::R(0, 360), lifespan = Op(1000);
  Op x = Op(0), y = Op(0);
  double scaleStart = 1, scaleEnd = 1;
  double alphaStart = 1, alphaEnd = 1;
  uint32_t tintStart = 0xffffff, tintEnd = 0xffffff;
  double gravityX = 0, gravityY = 0;
  double frequency = 0;
  int quantity = 1;
  int stopAfter = 0;
  int maxParticles = 0;
  bool emitting = true;
  Blend blend = Blend::Normal;
  std::function<void(double&, double&)> zone;      // random emit zone (relative point)
  std::function<void(Particle&)> emitCallback;
};

class ParticleEmitter : public GameObject {
 public:
  ParticleEmitter(Scene* s, Texture* t, const EmitterConfig& c);
  EmitterConfig cfg;
  Texture* tex;
  Op particleX, particleY;
  double timeScale = 1;
  bool emitting = true;
  std::vector<Particle> alive;

  void update(double deltaMs);                 // Phaser preUpdate
  void explode(int count, double x, double y);
  void start() { if (!emitting) { emitting = true; flowCounter_ = 0; } }
  void stop() { emitting = false; }
  bool finished() const { return !emitting && alive.empty(); }
  void render(Renderer& r, const Mat& world, double alpha) override;

 private:
  void emitParticle(int count, bool hasXY, double x, double y);
  Frame* frame_ = nullptr;
  double flowCounter_ = 0;
  int stopCounter_ = 0;
  int dead_ = 0;
  double frequency_;
};

}  // namespace px
