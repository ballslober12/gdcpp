// Tweens, easing and delayed calls (Phaser TweenManager / Clock equivalents).
#pragma once
#include <cmath>
#include <functional>
#include <memory>
#include <vector>
#include "math.h"

namespace px {

using EaseFn = std::function<double(double)>;

namespace ease {
inline double linear(double v) { return v; }
inline double quadOut(double v) { return v * (2 - v); }
inline double quadIn(double v) { return v * v; }
inline double quadInOut(double v) {
  v *= 2;
  if (v < 1) return 0.5 * v * v;
  --v;
  return -0.5 * (v * (v - 2) - 1);
}
inline double bounceOut(double v) {
  if (v < 1 / 2.75) return 7.5625 * v * v;
  if (v < 2 / 2.75) { v -= 1.5 / 2.75; return 7.5625 * v * v + 0.75; }
  if (v < 2.5 / 2.75) { v -= 2.25 / 2.75; return 7.5625 * v * v + 0.9375; }
  v -= 2.625 / 2.75;
  return 7.5625 * v * v + 0.984375;
}
inline double elasticOut(double v, double amplitude, double period) {
  if (v == 0) return 0;
  if (v == 1) return 1;
  double s = period / 4;
  if (amplitude < 1) amplitude = 1;
  else s = period * std::asin(1 / amplitude) / (2 * PI);
  return amplitude * std::pow(2, -10 * v) * std::sin((v - s) * (2 * PI) / period) + 1;
}
}  // namespace ease

struct Tween {
  const void* owner = nullptr;  // for killTweensOf
  double duration = 0;          // ms
  double delay = 0;             // ms
  EaseFn ease = ease::linear;
  bool yoyo = false;
  int repeat = 0;               // -1 = forever
  std::function<void()> onStart;                // called when delay elapsed (capture start values)
  std::function<void(double)> onUpdate;         // eased progress 0..1
  std::function<void()> onComplete;
  // runtime
  double elapsed = 0;
  bool started = false, forward = true, dead = false;
};

class TweenManager {
 public:
  Tween* add(Tween t) {
    pending_.push_back(std::make_unique<Tween>(std::move(t)));
    return pending_.back().get();
  }
  void killTweensOf(const void* owner) {
    for (auto& t : list_) if (t->owner == owner) t->dead = true;
    for (auto& t : pending_) if (t->owner == owner) t->dead = true;
  }
  void update(double deltaMs) {
    for (auto& t : pending_) list_.push_back(std::move(t));
    pending_.clear();
    for (size_t i = 0; i < list_.size(); i++) {
      Tween* t = list_[i].get();
      if (t->dead) continue;
      step(*t, deltaMs);
    }
    for (auto& t : pending_) list_.push_back(std::move(t));
    pending_.clear();
    for (size_t i = 0; i < list_.size();) {
      if (list_[i]->dead) list_.erase(list_.begin() + i);
      else i++;
    }
  }

 private:
  static void step(Tween& t, double dt) {
    if (!t.started) {
      t.delay -= dt;
      if (t.delay > 0) return;
      dt = -t.delay;
      t.delay = 0;
      t.started = true;
      if (t.onStart) t.onStart();
    }
    t.elapsed += dt;
    bool finishedLeg = t.elapsed >= t.duration;
    double p = t.duration > 0 ? std::min(t.elapsed / t.duration, 1.0) : 1.0;
    double q = t.forward ? p : 1 - p;
    if (t.onUpdate) t.onUpdate(t.ease(q));
    if (!finishedLeg) return;
    if (t.yoyo && t.forward) { t.forward = false; t.elapsed = 0; return; }
    if (t.repeat != 0) {
      if (t.repeat > 0) t.repeat--;
      t.forward = true; t.elapsed = 0;
      return;
    }
    t.dead = true;
    if (t.onComplete) t.onComplete();
  }
  std::vector<std::unique_ptr<Tween>> list_, pending_;
};

class Clock {
 public:
  void delayedCall(double ms, std::function<void()> fn) {
    pending_.push_back({ms, std::move(fn)});
  }
  void update(double deltaMs) {
    for (auto& e : pending_) events_.push_back(std::move(e));
    pending_.clear();
    for (auto& e : events_) e.remaining -= deltaMs;
    for (size_t i = 0; i < events_.size();) {
      if (events_[i].remaining <= 0) {
        auto fn = std::move(events_[i].fn);
        events_.erase(events_.begin() + i);
        if (fn) fn();
      } else i++;
    }
  }

 private:
  struct Ev { double remaining; std::function<void()> fn; };
  std::vector<Ev> events_, pending_;
};

}  // namespace px
