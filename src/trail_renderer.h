// Port of trail-renderer.js
#pragma once
#include <vector>
#include "engine/scene.h"

class TrailRenderer {
 public:
  TrailRenderer(px::Scene* scene, double fadeTime, double minSeg, double stroke, double maxSeg,
                uint32_t color = 16777215, double opacity = 1);
  void addToContainer(px::Container* c, double depth);
  void setPosition(double x, double y) { posX_ = x; posY_ = y; posInit_ = true; }
  void start() { active_ = true; }
  void stop() { active_ = false; }
  void reset();
  void update(double dt);

 private:
  struct Pt { double x, y, state; };
  uint32_t color_;
  double opacity_, fadeDelta_, minSegSq_, maxSeg_;
  size_t maxPoints_;
  double stroke_;
  std::vector<Pt> pts_;
  double posX_ = 0, posY_ = 0;
  bool posInit_ = false, active_ = false;
  px::Graphics* gfx_;
};
