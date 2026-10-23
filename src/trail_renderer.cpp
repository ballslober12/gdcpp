#include "trail_renderer.h"
#include <cmath>

TrailRenderer::TrailRenderer(px::Scene* scene, double fadeTime, double minSeg, double stroke,
                             double maxSeg, uint32_t color, double opacity)
    : color_(color), opacity_(opacity), fadeDelta_(1 / fadeTime), minSegSq_(minSeg * minSeg),
      maxSeg_(maxSeg), maxPoints_((size_t)(5 * std::floor(60 * fadeTime + 2))), stroke_(stroke) {
  gfx_ = scene->addGraphics();
  gfx_->setBlendMode(px::Blend::Add);
}

void TrailRenderer::addToContainer(px::Container* c, double depth) {
  c->add(gfx_);
  gfx_->setDepth(depth);
}

void TrailRenderer::reset() {
  pts_.clear();
  posInit_ = false;
  gfx_->clear();
}

void TrailRenderer::update(double dt) {
  if (!posInit_) { gfx_->clear(); return; }
  double fade = dt * fadeDelta_;
  size_t n = 0;
  for (size_t i = 0; i < pts_.size(); i++) {
    pts_[i].state -= fade;
    if (pts_[i].state > 0) {
      if (n != i) pts_[n] = pts_[i];
      n++;
    }
  }
  pts_.resize(n);
  if (active_ && pts_.size() < maxPoints_) {
    size_t len = pts_.size();
    bool add = true;
    if (len > 0) {
      const Pt& p = pts_[len - 1];
      double dx = posX_ - p.x, dy = posY_ - p.y, d2 = dx * dx + dy * dy;
      if (maxSeg_ > 0 && std::sqrt(d2) > maxSeg_) pts_.clear();
      else if (d2 < minSegSq_) add = false;
      else if (len > 1) {
        const Pt& p2 = pts_[len - 2];
        double ex = posX_ - p2.x, ey = posY_ - p2.y;
        if (ex * ex + ey * ey < 2 * minSegSq_) add = false;
      }
    }
    if (add) pts_.push_back({posX_, posY_, 1});
  }
  gfx_->clear();
  size_t len = pts_.size();
  if (len < 2) return;
  for (size_t i = 0; i + 1 < len; i++) {
    const Pt& a = pts_[i];
    const Pt& b = pts_[i + 1];
    double alpha = 0.5 * (a.state + b.state) * opacity_;
    gfx_->lineStyle(stroke_, color_, alpha);
    gfx_->lineBetween(a.x, a.y, b.x, b.y);
  }
}
