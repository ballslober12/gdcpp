// Port of tween-value.js + color-manager.js
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include "constants.h"

struct RGB { int r = 255, g = 255, b = 255; };

class TweenValue {
 public:
  TweenValue(RGB from_, RGB to_, double duration_)
      : from(from_), to(to_), duration(duration_), done(duration_ <= 0),
        current(duration_ <= 0 ? to_ : from_) {}
  RGB from, to;
  double duration, elapsed = 0;
  bool done;
  RGB current;
  void step(double dt) {
    if (done) return;
    elapsed += dt;
    double p = duration > 0 ? std::min(elapsed / duration, 1.0) : 1;
    if (p >= 1) { current = to; done = true; return; }
    auto lerp = [&](int a, int b) { return (int)std::floor(a + (b - a) * p + 0.5); };
    current = {lerp(from.r, to.r), lerp(from.g, to.g), lerp(from.b, to.b)};
  }
};

class ColorManager {
 public:
  ColorManager() { reset(); }
  void reset() {
    colors_.clear(); actions_.clear();
    colors_[num30] = {0, 102, 255};
    colors_[num31] = {0, 68, 170};
  }
  void triggerColor(int channel, RGB to, double duration) {
    RGB from = getColor(channel);
    actions_.erase(channel);
    actions_.emplace(channel, TweenValue(from, to, duration));
    if (duration <= 0) colors_[channel] = to;
  }
  void step(double dt) {
    for (auto it = actions_.begin(); it != actions_.end();) {
      it->second.step(dt);
      colors_[it->first] = it->second.current;
      if (it->second.done) it = actions_.erase(it);
      else ++it;
    }
  }
  RGB getColor(int channel) const {
    auto it = colors_.find(channel);
    return it == colors_.end() ? RGB{} : it->second;
  }
  uint32_t getHex(int channel) const {
    RGB c = getColor(channel);
    return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.b;
  }

 private:
  std::map<int, RGB> colors_;
  std::map<int, TweenValue> actions_;
};
