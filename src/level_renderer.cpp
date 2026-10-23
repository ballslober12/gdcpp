#include "level_renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace px;

namespace {
std::string replaceFirst(std::string s, const std::string& a, const std::string& b) {
  size_t p = s.find(a);
  if (p != std::string::npos) s.replace(p, a.size(), b);
  return s;
}
bool endsWith(const std::string& s, const std::string& e) {
  return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0;
}
double rawNum(const LevelItem& it, int k, double def, bool isInt) {
  auto f = it.raw.find(k);
  if (f == it.raw.end()) return def;  // JS `??`
  return isInt ? (double)std::strtol(f->second.c_str(), nullptr, 10)
               : std::strtod(f->second.c_str(), nullptr);
}
}  // namespace

LevelRenderer::LevelRenderer(Scene* scene, FloatRef* cameraXRef)
    : scene_(scene), cameraXRef_(cameraXRef), groundStartScreenY_(flipY(0)) {
  additiveContainer = scene->addContainer(0, 0);
  additiveContainer->setDepth(-1);
  container = scene->addContainer(0, 0);
  topContainer = scene->addContainer(0, 0);
  topContainer->setDepth(13);
  buildGround();
}

void LevelRenderer::loadLevel(const std::string& levelText) {
  std::vector<LevelItem> items;
  parseLevelString(levelText, items);
  spawnLevelObjects(items);
}

void LevelRenderer::buildGround() {
  Scene* scene = scene_;
  Texture* sheet = scene->app->textures->get("GJ_WebSheet");
  Frame* f = sheet ? sheet->get("groundSquare_01_001.png") : nullptr;
  tileW_ = f ? f->width : 1012;
  int n = (int)std::ceil(screenWidth / tileW_) + 2;
  double gy = flipY(0);
  double startX = -groundYOffset;
  for (int i = 0; i < n; i++) {
    double wx = startX + i * tileW_;
    Image* im = scene->addImage(0, gy, "GJ_WebSheet", "groundSquare_01_001.png");
    im->setOrigin(0, 0); im->setTint(17578); im->setDepth(20); im->worldX = wx;
    groundTiles_.push_back(im);
    Image* im2 = scene->addImage(0, gy, "GJ_WebSheet", "groundSquare_01_001.png");
    im2->setOrigin(0, 1); im2->setFlipY(true); im2->setTint(17578); im2->setDepth(20);
    im2->setVisible(false); im2->worldX = wx;
    ceilingTiles_.push_back(im2);
  }
  maxGroundWorldX_ = startX + (n - 1) * tileW_;
  Frame* fl = sheet ? sheet->get("floorLine_01_001.png") : nullptr;
  double lineW = fl ? fl->width : 888;
  double sc = screenWidth / lineW;
  groundLine_ = scene->addImage(screenWidth / 2, gy - 1, "GJ_WebSheet", "floorLine_01_001.png");
  groundLine_->setOrigin(0.5, 0); groundLine_->setScale(sc, 1);
  groundLine_->setBlendMode(Blend::Add); groundLine_->setDepth(21);
  ceilingLine_ = scene->addImage(screenWidth / 2, gy + 1, "GJ_WebSheet", "floorLine_01_001.png");
  ceilingLine_->setOrigin(0.5, 1); ceilingLine_->setScale(sc, 1); ceilingLine_->setFlipY(true);
  ceilingLine_->setBlendMode(Blend::Add); ceilingLine_->setDepth(21); ceilingLine_->setVisible(false);
  const double a = 100.0 / 255;
  groundShadowL_ = scene->addImage(-1, gy, "GJ_WebSheet", "groundSquareShadow_001.png");
  groundShadowL_->setOrigin(0, 0); groundShadowL_->setDepth(22); groundShadowL_->setAlpha(a);
  groundShadowL_->setScale(0.7, 1); groundShadowL_->setBlendMode(Blend::DstColor);
  groundShadowR_ = scene->addImage(screenWidth + 1, gy, "GJ_WebSheet", "groundSquareShadow_001.png");
  groundShadowR_->setOrigin(1, 0); groundShadowR_->setDepth(22); groundShadowR_->setAlpha(a);
  groundShadowR_->setScale(0.7, 1); groundShadowR_->setFlipX(true); groundShadowR_->setBlendMode(Blend::DstColor);
  ceilingShadowL_ = scene->addImage(-1, gy, "GJ_WebSheet", "groundSquareShadow_001.png");
  ceilingShadowL_->setOrigin(0, 1); ceilingShadowL_->setDepth(22); ceilingShadowL_->setAlpha(a);
  ceilingShadowL_->setScale(0.7, 1); ceilingShadowL_->setFlipY(true);
  ceilingShadowL_->setBlendMode(Blend::DstColor); ceilingShadowL_->setVisible(false);
  ceilingShadowR_ = scene->addImage(screenWidth + 1, gy, "GJ_WebSheet", "groundSquareShadow_001.png");
  ceilingShadowR_->setOrigin(1, 1); ceilingShadowR_->setDepth(22); ceilingShadowR_->setAlpha(a);
  ceilingShadowR_->setScale(0.7, 1); ceilingShadowR_->setFlipX(true); ceilingShadowR_->setFlipY(true);
  ceilingShadowR_->setBlendMode(Blend::DstColor); ceilingShadowR_->setVisible(false);
}

void LevelRenderer::resizeScreen() {
  Scene* scene = scene_;
  int n = (int)std::ceil(screenWidth / tileW_) + 2;
  double gy = flipY(0);
  while ((int)groundTiles_.size() < n) {
    double wx = maxGroundWorldX_ + tileW_;
    uint32_t t0 = groundTiles_.empty() ? 17578 : (groundTiles_[0]->tint ? groundTiles_[0]->tint : 17578);
    Image* im = scene->addImage(0, gy, "GJ_WebSheet", "groundSquare_01_001.png");
    im->setOrigin(0, 0); im->setTint(t0); im->setDepth(20); im->worldX = wx;
    groundTiles_.push_back(im);
    Image* im2 = scene->addImage(0, gy, "GJ_WebSheet", "groundSquare_01_001.png");
    im2->setOrigin(0, 1); im2->setFlipY(true); im2->setTint(t0); im2->setDepth(20);
    im2->setVisible(false); im2->worldX = wx;
    ceilingTiles_.push_back(im2);
    maxGroundWorldX_ = wx;
  }
  Frame* fl = scene->app->textures->get("GJ_WebSheet")->get("floorLine_01_001.png");
  double sc = screenWidth / (fl ? fl->width : 888);
  groundLine_->x = screenWidth / 2; groundLine_->setScale(sc, 1);
  ceilingLine_->x = screenWidth / 2; ceilingLine_->setScale(sc, 1);
  groundShadowR_->x = screenWidth + 1;
  ceilingShadowR_->x = screenWidth + 1;
}

void LevelRenderer::updateGroundTiles(double yOffset) {
  double camX = cameraXRef_->value;
  double maxX = maxGroundWorldX_ ? maxGroundWorldX_ : -INFINITY;
  double gy, cy;
  if (flyGroundActive_ && groundTargetValue_ > 0.001) {
    double t = groundTargetValue_;
    gy = groundStartScreenY_ + (620 - groundStartScreenY_) * t;
    cy = ceilingStartScreenY_ + (20 - ceilingStartScreenY_) * t;
    double lim = flipY(0) + yOffset;
    if (gy > lim) gy = lim;
  } else {
    gy = flipY(0) + yOffset;
    cy = 0;
  }
  for (size_t i = 0; i < groundTiles_.size(); i++) {
    Image* g = groundTiles_[i];
    Image* c = ceilingTiles_[i];
    if (g->worldX + tileW_ <= camX) {
      g->worldX = maxX + tileW_;
      c->worldX = g->worldX;
      maxX = g->worldX;
      maxGroundWorldX_ = maxX;
    }
    double sx = g->worldX - camX;
    g->x = sx; g->y = gy;
    c->x = sx; c->y = cy;
    c->setVisible(flyGroundActive_ && groundTargetValue_ > 0);
  }
  groundLine_->y = gy;
  if (flyGroundActive_ && groundTargetValue_ > 0) {
    ceilingLine_->y = cy;
    ceilingLine_->setVisible(true);
  } else ceilingLine_->setVisible(false);
  groundShadowL_->y = gy;
  groundShadowR_->y = gy;
  bool vis = flyGroundActive_ && groundTargetValue_ > 0;
  ceilingShadowL_->y = cy; ceilingShadowR_->y = cy;
  ceilingShadowL_->setVisible(vis); ceilingShadowR_->setVisible(vis);
}

void LevelRenderer::shiftGroundTiles(double d) {
  for (size_t i = 0; i < groundTiles_.size(); i++) {
    groundTiles_[i]->worldX += d;
    ceilingTiles_[i]->worldX += d;
  }
  maxGroundWorldX_ += d;
}

void LevelRenderer::resetGroundTiles(double startX) {
  for (size_t i = 0; i < groundTiles_.size(); i++) {
    groundTiles_[i]->worldX = startX + i * tileW_;
    ceilingTiles_[i]->worldX = startX + i * tileW_;
  }
  maxGroundWorldX_ = startX + (groundTiles_.size() - 1) * tileW_;
  resetGroundState();
}

void LevelRenderer::resetGroundState() {
  flyGroundActive_ = false; groundTargetValue_ = 0; groundAnimating_ = false;
  groundY_ = 0; flyCameraTarget.reset();
}

void LevelRenderer::computeFlyBounds(double y, double& floorY, double& ceilY) const {
  double v = y - 300;
  v = std::floor(v / baseUnit) * baseUnit;
  v = std::max(0.0, v);
  floorY = v;
  ceilY = v + physicsConst600;
}

void LevelRenderer::setFlyMode(bool on, double playerY) {
  if (on) {
    computeFlyBounds(playerY, flyFloorY_, flyCeilingY_);
    flyGroundActive_ = true;
    double v = flyFloorY_ + 300;
    double t = v - 320 + unusedConst180;
    if (t < 0) t = 0;
    flyCameraTarget = t;
    double camY = cameraYRef ? *cameraYRef : 0;
    groundStartScreenY_ = flipY(0) + camY;
    ceilingStartScreenY_ = 0;
    groundAnimFrom_ = groundTargetValue_; groundAnimTo_ = 1; groundAnimTime_ = 0;
    groundAnimDuration_ = 0.5; groundAnimating_ = true;
  } else {
    flyCameraTarget.reset();
    groundAnimFrom_ = groundTargetValue_; groundAnimTo_ = 0; groundAnimTime_ = 0;
    groundAnimDuration_ = 0.5; groundAnimating_ = true;
  }
}

void LevelRenderer::stepGroundAnimation(double dt) {
  if (!groundAnimating_) return;
  groundAnimTime_ += dt;
  double p = groundAnimDuration_ > 0 ? std::min(groundAnimTime_ / groundAnimDuration_, 1.0) : 1;
  groundTargetValue_ = groundAnimFrom_ + (groundAnimTo_ - groundAnimFrom_) * p;
  if (p >= 1) {
    groundAnimating_ = false;
    groundTargetValue_ = groundAnimTo_;
    if (groundAnimTo_ == 0) flyGroundActive_ = false;
  }
}

void LevelRenderer::applyVisualProps(Image* g, const std::string&, const LevelItem& item,
                                     const bool* hasTint, uint32_t tint, bool black) {
  if (!g) return;
  if (item.flipX) g->setFlipX(true);
  if (item.flipY) g->setFlipY(true);
  double rot = 0 + item.rot;  // gjBaseRotationDeg is never set
  if (rot != 0) g->setAngle(rot);
  if (item.scale != 1) g->setScale(item.scale);
  if (hasTint) {
    if (*hasTint) g->setTint(tint);
    else if (black) g->setTint(0);
  }
}

void LevelRenderer::addVisualSprite(GameObject* g, const ObjDef* def, bool portalFront) {
  if (!g) return;
  if (def && def->additive) { g->setBlendMode(Blend::Add); g->eeLayer = 0; }
  else if (def && portalFront) g->eeLayer = 2;
  else if (def && def->hasZ && def->z < 0) g->eeLayer = 0;
  else g->eeLayer = 1;
}

std::string LevelRenderer::getGlowFrameName(const std::string& f, bool& ok) {
  ok = endsWith(f, "_001.png");
  return ok ? replaceFirst(f, "_001.png", "_glow_001.png") : std::string();
}

void LevelRenderer::addGlowSprite(Scene* scene, double x, double y, const std::string& frame,
                                  const LevelItem& item, double worldX) {
  bool ok;
  std::string gname = getGlowFrameName(frame, ok);
  if (!ok) return;
  if (!findAtlasFrame(scene, gname).ok && !scene->app->textures->exists(gname)) return;
  Image* im = addImageFromAtlas(scene, x, y, gname);
  if (!im) return;
  bool none = false;
  applyVisualProps(im, gname, item, nullptr, 0, false);
  (void)none;
  im->setBlendMode(Blend::Add);
  im->eeLayer = 0;
  im->eeWorldX = worldX;
  im->eeBaseY = y;
  addToSection(im);
}

void LevelRenderer::spawnLevelObjects(const std::vector<LevelItem>& items) {
  Scene* scene = scene_;
  lastObjectX_ = 0;
  for (const LevelItem& item : items) {
    const ObjDef* def = getObjDef(item.id);
    if (def && def->type == DefType::Trigger) {
      if (item.id == 29 || item.id == 30) {
        ColorTrigger t;
        t.x = 2 * item.x;
        t.index = item.id == 29 ? 1000 : 1001;
        t.color = {(int)rawNum(item, 7, 255, true), (int)rawNum(item, 8, 255, true),
                   (int)rawNum(item, 9, 255, true)};
        t.duration = rawNum(item, 10, 0, false);
        auto f = item.raw.find(14);
        t.tintGround = f != item.raw.end() && f->second == "1";
        colorTriggers_.push_back(t);
      }
      if (def->enterEffect) enterEffectTriggers_.push_back({2 * item.x, def->enterEffect});
      continue;
    }
    double wx = 2 * item.x, wy = 2 * item.y;
    if (wx > lastObjectX_) lastObjectX_ = wx;
    std::string frame = (def && def->hasFrame) ? def->frame : std::string();
    if (def && !def->randomFrames.empty())
      frame = def->randomFrames[(size_t)std::floor(random01() * def->randomFrames.size())];
    if (!frame.empty()) {
      double px_ = wx, py_ = flipY(wy);
      bool isFront = (def->type == DefType::Portal || def->type == DefType::Speed) &&
                     frame.find("_front_") != std::string::npos;
      if (isFront) {
        std::string back = replaceFirst(frame, "_front_", "_back_");
        Image* b = addImageFromAtlas(scene, px_, py_, back);
        if (b) {
          applyVisualProps(b, back, item, nullptr, 0, false);
          b->eeLayer = 1; b->eeWorldX = wx; b->eeBaseY = py_;
          addToSection(b);
        }
      }
      if (def->glow) addGlowSprite(scene, px_, py_, frame, item, wx);
      Image* main = addImageFromAtlas(scene, px_, py_, frame);
      if (main) {
        bool ht = def->hasTint;
        applyVisualProps(main, frame, item, &ht, def->tint, def->black);
        addVisualSprite(main, def, isFront);
        main->eeWorldX = wx; main->eeBaseY = py_;
        addToSection(main);
      }
      if (def->type == DefType::Solid || def->type == DefType::Hazard) {
        std::string f2 = replaceFirst(frame, "_001.png", "_2_001.png");
        Image* o = findAtlasFrame(scene, f2).ok ? addImageFromAtlas(scene, px_, py_, f2) : nullptr;
        if (o) {
          applyVisualProps(o, f2, item, nullptr, 0, false);
          addVisualSprite(o, nullptr, false);
          o->eeWorldX = wx; o->eeBaseY = py_;
          addToSection(o);
        }
      }
      for (const ChildDef& c : def->children) {
        double dx = 0, dy = 0;
        if (c.hasLocalDy) {
          double lx = 0, ly = c.localDy;
          if (item.flipX) lx = -lx;
          if (item.flipY) ly = -ly;
          double r = (item.rot * PI) / 180;
          dx = lx * std::cos(r) - ly * std::sin(r);
          dy = lx * std::sin(r) + ly * std::cos(r);
        }
        Image* ci = addImageFromAtlas(scene, px_ + dx, py_ + dy, c.frame);
        if (ci) {
          bool ht = c.hasTint;
          applyVisualProps(ci, c.frame, item, &ht, c.tint, false);
          if (c.audioScale) {
            ci->setScale(0.1); ci->setAlpha(0.9); ci->eeAudioScale = true;
            audioScaleSprites_.push_back(ci);
          }
          if ((c.hasZ ? c.z : -1) < 0) { ci->eeLayer = 1; ci->eeBehindParent = true; }
          else {
            // _addVisualSprite(sprite, childDef): additive / z<0 / default
            if (c.additive) { ci->setBlendMode(Blend::Add); ci->eeLayer = 0; }
            else ci->eeLayer = 1;
          }
          ci->eeWorldX = wx + dx; ci->eeBaseY = py_ + dy;
          addToSection(ci);
        }
      }
    }
    if (def && def->portalParticle && !frame.empty()) {
      double px_ = wx, py_ = flipY(wy);
      const double k = 2;
      double ex = px_ - 5 * k, ey = py_;
      EmitterConfig cfg;
      cfg.frame = "square.png";
      cfg.lifespan = Op::R(200, 1000);
      cfg.speed = Op(0);
      cfg.scaleStart = 0.75; cfg.scaleEnd = 0.125;
      cfg.alphaStart = 0.5; cfg.alphaEnd = 0;
      cfg.tintStart = cfg.tintEnd = def->portalParticleColor;
      cfg.blend = Blend::Add;
      cfg.frequency = 20; cfg.maxParticles = 0; cfg.emitting = true;
      cfg.zone = [k](double& x, double& y) {
        double a = ((190 * random01() + 85) * PI) / 180;
        double r = 20 * k + 40 * random01() * k;
        x = std::cos(a) * r; y = std::sin(a) * r;
      };
      cfg.emitCallback = [](Particle& p) {
        double vx = -p.x, vy = -p.y;
        double len = std::sqrt(vx * vx + vy * vy);
        if (len == 0) len = 1;
        double life = p.life / 1000.0;
        double sp = (len - 20) / (life != 0 ? life : 0.3);
        p.vx = (vx / len) * sp;
        p.vy = (vy / len) * sp;
      };
      ParticleEmitter* em = scene->addParticles(ex, ey, "GJ_WebSheet", cfg);
      em->setDepth(14);
      em->eeLayer = 2; em->eeWorldX = wx; em->eeBaseY = ey;
      addToSection(em);
    }
    if (def) {
      if (def->type == DefType::Solid && def->gridW > 0 && def->gridH > 0) {
        auto o = std::make_unique<LevelObject>(ObjKind::Solid, wx, wy, def->gridW * baseUnit,
                                               def->gridH * baseUnit);
        addCollisionToSection(o.get());
        objects.push_back(std::move(o));
      } else if (def->type == DefType::Hazard) {
        double w = 0, h = 0;
        if (def->hasSprite && def->spriteW > 0 && def->spriteH > 0 && def->hasHitbox) {
          w = def->spriteW * def->hitboxScaleX * 2;
          h = def->spriteH * def->hitboxScaleY * 2;
        } else if (def->gridW > 0 && def->gridH > 0) {
          w = 12 * def->gridW; h = 24 * def->gridH;
        }
        if (w > 0 && h > 0) {
          auto o = std::make_unique<LevelObject>(ObjKind::Hazard, wx, wy, w, h);
          addCollisionToSection(o.get());
          objects.push_back(std::move(o));
        }
      } else if (def->type == DefType::Portal) {
        bool has = false;
        ObjKind k = ObjKind::PortalFly;
        if (def->sub == "fly") { k = ObjKind::PortalFly; has = true; }
        else if (def->sub == "cube") { k = ObjKind::PortalCube; has = true; }
        if (has) {
          auto o = std::make_unique<LevelObject>(k, wx, wy, 90, def->gridH * baseUnit);
          o->portalY = wy;
          addCollisionToSection(o.get());
          objects.push_back(std::move(o));
        }
      }
    }
  }
  std::stable_sort(colorTriggers_.begin(), colorTriggers_.end(),
                   [](const ColorTrigger& a, const ColorTrigger& b) { return a.x < b.x; });
  std::stable_sort(enterEffectTriggers_.begin(), enterEffectTriggers_.end(),
                   [](const EnterEffectTrigger& a, const EnterEffectTrigger& b) { return a.x < b.x; });
  endXPos = std::max(screenWidth + 1200, lastObjectX_ + 680);
}

void LevelRenderer::createEndPortal(Scene* scene) {
  if (endXPos <= 0) return;
  double ex = endXPos, gy = flipY(240);
  int n = 16;
  endPortalContainer_ = scene->addContainer(ex, gy);
  for (int i = 0; i < n; i++) {
    Image* im = scene->addImage(0, (i - std::floor(n / 2.0)) * baseUnit, "GJ_WebSheet", "square_02_001.png");
    im->setAngle(-90);
    endPortalContainer_->add(im);
  }
  container->add(endPortalContainer_);
  endPortalShine_ = scene->addImage(ex - 58, gy, "GJ_WebSheet", "gradientBar.png");
  Frame* gf = scene->app->textures->get("GJ_WebSheet")->get("gradientBar.png");
  double gh = (gf && gf->height) ? gf->height : 64;
  endPortalShine_->setBlendMode(Blend::Add);
  endPortalShine_->setTint(colorGreenTint);
  endPortalShine_->setScale(1, 960 / gh);
  additiveContainer->add(endPortalShine_);
  EmitterConfig cfg;
  cfg.frame = "square.png";
  cfg.lifespan = Op::R(200, 1000);
  cfg.speed = Op(0);
  cfg.scaleStart = 0.75; cfg.scaleEnd = 0.125;
  cfg.alphaStart = 1; cfg.alphaEnd = 0;
  cfg.tintStart = cfg.tintEnd = colorGreenTint;
  cfg.blend = Blend::Add;
  cfg.frequency = 10; cfg.maxParticles = 100; cfg.emitting = true;
  cfg.zone = [](double& x, double& y) {
    double a = ((85 + 190 * random01()) * PI) / 180;
    double r = 320 + 80 * (2 * random01() - 1);
    x = std::cos(a) * r; y = std::sin(a) * r;
  };
  cfg.emitCallback = [](Particle& p) {
    double vx = -p.x, vy = -p.y;
    double len = std::sqrt(vx * vx + vy * vy);
    if (len == 0) len = 1;
    double lf = p.life / 1000.0;
    double sp = (len - 20) / (lf != 0 ? lf : 0.3);
    p.vx = (vx / len) * sp;
    p.vy = (vy / len) * sp;
  };
  endPortalEmitter_ = scene->addParticles(ex - 30, gy, "GJ_WebSheet", cfg);
  endPortalEmitter_->setDepth(14);
  topContainer->add(endPortalEmitter_);
  endPortalGameY_ = 240;
}

void LevelRenderer::updateEndPortalY(double a, bool other) {
  if (!endPortalContainer_) return;
  double v = 140 + a;
  double y = other ? v : std::max(240.0, v);
  double gy = flipY(y);
  endPortalContainer_->y = gy;
  endPortalShine_->y = gy;
  endPortalEmitter_->y = gy;
  endPortalGameY_ = y;
}

std::vector<ColorTrigger> LevelRenderer::checkColorTriggers(double x) {
  std::vector<ColorTrigger> out;
  while (colorTriggerIdx_ < colorTriggers_.size()) {
    const ColorTrigger& t = colorTriggers_[colorTriggerIdx_];
    if (!(t.x <= x)) break;
    out.push_back(t);
    colorTriggerIdx_++;
  }
  return out;
}

void LevelRenderer::addToSection(GameObject* o) {
  int idx = (int)std::max(0.0, std::floor(o->eeWorldX / 400));
  if ((int)sections_.size() <= idx) sections_.resize(idx + 1);
  sections_[idx].push_back(o);
  int layer = o->eeLayer;
  if (layer == 2) { topContainer->add(o); return; }
  if ((int)sectionContainers_.size() <= idx) sectionContainers_.resize(idx + 1);
  if (!sectionContainers_[idx]) {
    auto s = std::make_unique<Section>();
    s->additive = scene_->addContainer(0, 0);
    s->normal = scene_->addContainer(0, 0);
    additiveContainer->add(s->additive);
    container->add(s->normal);
    sectionContainers_[idx] = std::move(s);
  }
  Section* s = sectionContainers_[idx].get();
  if (layer == 0) s->additive->add(o);
  else if (o->eeBehindParent) s->normal->addAt(o, 0);
  else s->normal->add(o);
}

void LevelRenderer::addCollisionToSection(LevelObject* o) {
  int idx = (int)std::max(0.0, std::floor(o->x / 400));
  if ((int)collisionSections_.size() <= idx) collisionSections_.resize(idx + 1);
  collisionSections_[idx].push_back(o);
}

void LevelRenderer::setSectionVisible(int i, bool v) {
  if (i < 0 || i >= (int)sectionContainers_.size() || !sectionContainers_[i]) return;
  sectionContainers_[i]->additive->visible = v;
  sectionContainers_[i]->normal->visible = v;
}

void LevelRenderer::updateVisibility(double cameraX) {
  int last = (int)sectionContainers_.size() - 1;
  if (last < 0) return;
  int lo = (int)std::max(0.0, std::floor((cameraX - 140) / 400));
  int hi = (int)std::min<double>(last, std::floor((cameraX + screenWidth + 140) / 400));
  int vmin = visMinSec_, vmax = visMaxSec_;
  if (vmin < 0) {
    for (int i = 0; i <= last; i++) setSectionVisible(i, i >= lo && i <= hi);
    visMinSec_ = lo; visMaxSec_ = hi;
    return;
  }
  if (lo != vmin || hi != vmax) {
    if (lo > vmin) for (int i = vmin; i <= std::min(lo - 1, vmax); i++) setSectionVisible(i, false);
    if (hi < vmax) for (int i = std::max(hi + 1, vmin); i <= vmax; i++) setSectionVisible(i, false);
    if (lo < vmin) for (int i = lo; i <= std::min(vmin - 1, hi); i++) setSectionVisible(i, true);
    if (hi > vmax) for (int i = std::max(vmax + 1, lo); i <= hi; i++) setSectionVisible(i, true);
    visMinSec_ = lo; visMaxSec_ = hi;
  }
}

const std::vector<LevelObject*>& LevelRenderer::getNearbySectionObjects(double x) {
  int s = (int)std::max(0.0, std::floor(x / 400));
  int lo = std::max(0, s - 1);
  int hi = std::min((int)collisionSections_.size() - 1, s + 1);
  nearbyBuffer_.clear();
  for (int i = lo; i <= hi; i++)
    for (LevelObject* o : collisionSections_[i]) nearbyBuffer_.push_back(o);
  return nearbyBuffer_;
}

void LevelRenderer::checkEnterEffectTriggers(double x) {
  while (enterEffectTriggerIdx_ < enterEffectTriggers_.size()) {
    const EnterEffectTrigger& t = enterEffectTriggers_[enterEffectTriggerIdx_];
    if (!(t.x <= x)) break;
    activeEnterEffect_ = t.effect;
    activeExitEffect_ = t.effect;
    enterEffectTriggerIdx_++;
  }
}

void LevelRenderer::resetEnterEffectTriggers() {
  enterEffectTriggerIdx_ = 0; activeEnterEffect_ = 0; activeExitEffect_ = 0;
  for (size_t i = 0; i < sections_.size(); i++) {
    setSectionVisible((int)i, true);
    for (GameObject* p : sections_[i]) {
      p->eeActive = false; p->visible = true;
      p->x = p->eeWorldX; p->y = p->eeBaseY;
      if (!p->eeAudioScale) p->setScale(1);
      p->setAlpha(1);
    }
  }
}

void LevelRenderer::applyEnterEffects(double cameraX) {
  const double n32 = 400, n33 = 140, n34 = 200;
  double left = cameraX, right = cameraX + screenWidth, mid = cameraX + screenWidth / 2;
  int lo = (int)std::max(0.0, std::floor((left - n33) / n32));
  int hi = (int)std::min<double>((double)sections_.size() - 1, std::floor((right + n33) / n32));
  auto restore = [](GameObject* p) {
    if (p->eeActive) {
      p->eeActive = false;
      p->y = p->eeBaseY; p->x = p->eeWorldX;
      if (!p->eeAudioScale) p->setScale(1);
      p->setAlpha(1);
    }
  };
  for (int i = lo; i <= hi; i++) {
    auto& sec = sections_[i];
    if (sec.empty()) continue;
    double secX = i * n32;
    bool fullyInside = secX >= left + n33 && secX + n32 <= right - n33;
    for (GameObject* p : sec) {
      if (fullyInside) { restore(p); continue; }
      double wx = p->eeWorldX;
      bool rightSide = wx > mid;
      double t = rightSide ? std::max(0.0, std::min(1.0, (right - wx) / n33))
                           : std::max(0.0, std::min(1.0, (wx - left) / n33));
      if (t >= 1) { restore(p); continue; }
      p->eeActive = true;
      int effect = rightSide ? activeEnterEffect_ : activeExitEffect_;
      double inv = 1 - t;
      double by = p->eeBaseY, bx = p->eeWorldX, a = t, sc = 1;
      switch (effect) {
        case 0: break;
        case 1: by = p->eeBaseY + n34 * inv; break;
        case 2: by = p->eeBaseY - n34 * inv; break;
        case 3: bx = p->eeWorldX - n34 * inv; break;
        case 4: bx = p->eeWorldX + n34 * inv; break;
        case 5: if (!p->eeAudioScale) sc = t; break;
        case 6: if (!p->eeAudioScale) sc = 1 + 0.75 * inv; break;
      }
      if (p->x != bx) p->x = bx;
      if (p->y != by) p->y = by;
      if (p->alpha != a) p->alpha = a;
      if (!p->eeAudioScale && p->scaleX != sc) p->setScale(sc);
    }
  }
}

void LevelRenderer::setGroundColor(uint32_t c) {
  for (Image* g : groundTiles_) g->setTint(c);
  for (Image* g : ceilingTiles_) g->setTint(c);
}

void LevelRenderer::updateAudioScale(double s) {
  for (GameObject* g : audioScaleSprites_) g->setScale(s);
}

void LevelRenderer::resetObjects() {
  for (auto& o : objects) o->activated = false;
  for (GameObject* g : audioScaleSprites_) g->setScale(0.1);
}
