#include "player.h"
#include <cmath>
#include "game_scene.h"
#include "win_effects.h"

using namespace px;

namespace {
Image* layeredOrNull(Scene* s, double x, double y, const char* name, double depth, bool visible) {
  return createLayeredSprite(s, x, y, name, depth, visible);
}
}  // namespace

Player::Player(GameScene* scene, PlayerPhysicsState* state, LevelRenderer* gameLayer)
    : p(state), scene_(scene), gameLayer_(gameLayer) {
  createSprites();
  initParticles();
}

Player::~Player() { cleanupExplosion(); }

void Player::createSprites() {
  Scene* scene = scene_;
  double gy = flipY(p->y), gx = groundYOffset;
  Image* pg = layeredOrNull(scene, gx, gy, "player_01_glow_001.png", 9, false);
  Image* ps = layeredOrNull(scene, gx, gy, "player_01_001.png", 10, true);
  Image* po = layeredOrNull(scene, gx, gy, "player_01_2_001.png", 8, true);
  Image* pe = layeredOrNull(scene, gx, gy, "player_01_extra_001.png", 12, true);
  playerGlow_ = pg; playerOverlay_ = po; playerExtra_ = pe;
  if (pg) { pg->setTint(colorCyanTint); pg->glowEnabled = false; }
  if (ps) { ps->setTint(colorGreenTint); playerSprite_ = ps; }
  else {
    Rectangle* r = scene->addRectangle(gx, gy, baseUnit, baseUnit, colorGreenTint);
    r->setDepth(10);
    playerSprite_ = r;
  }
  if (po) po->setTint(colorCyanTint);
  Image* sg = layeredOrNull(scene, gx, gy, "ship_01_glow_001.png", 9, false);
  Image* ss = layeredOrNull(scene, gx, gy, "ship_01_001.png", 10, false);
  Image* so = layeredOrNull(scene, gx, gy, "ship_01_2_001.png", 8, false);
  Image* se = layeredOrNull(scene, gx, gy, "ship_01_extra_001.png", 12, false);
  shipGlow_ = sg; shipOverlay_ = so; shipExtra_ = se;
  if (sg) { sg->setTint(colorCyanTint); sg->glowEnabled = false; }
  if (ss) { ss->setTint(colorGreenTint); shipSprite_ = ss; }
  else {
    Rectangle* r = scene->addRectangle(gx, gy, 144, 80, colorGreenTint);  // polygon fallback
    r->setDepth(10)->setVisible(false);
    shipSprite_ = r;
  }
  if (so) so->setTint(colorCyanTint);
  playerLayers_ = {playerSprite_, playerGlow_, playerOverlay_, playerExtra_};
  shipLayers_ = {shipSprite_, shipGlow_, shipOverlay_, shipExtra_};
  allLayers_ = playerLayers_;
  allLayers_.insert(allLayers_.end(), shipLayers_.begin(), shipLayers_.end());
}

void Player::initParticles() {
  Scene* s = scene_;
  {
    EmitterConfig c;
    c.speed = Op::R(110, 190); c.angle = Op::R(225, 315); c.lifespan = Op::R(150, 450);
    c.scaleStart = 0.5; c.scaleEnd = 0; c.gravityY = 600; c.frequency = 1000.0 / 30;
    c.blend = Blend::Add; c.alphaStart = 1; c.alphaEnd = 0;
    c.tintStart = c.tintEnd = colorGreenTint;
    particleEmitter_ = s->addParticles(0, 0, "GJ_WebSheet", c);
    particleEmitter_->stop();
    particleEmitter_->setDepth(9);
    gameLayer_->container->add(particleEmitter_);
  }
  {
    EmitterConfig c;
    c.speed = Op::R(22, 38); c.angle = Op::R(225, 315); c.lifespan = Op::R(150, 450);
    c.scaleStart = 0.5; c.scaleEnd = 0; c.gravityY = 600; c.frequency = 1000.0 / 30;
    c.blend = Blend::Add; c.tintStart = 16737280; c.tintEnd = 16711680;
    c.alphaStart = 1; c.alphaEnd = 0;
    flyParticleEmitter_ = s->addParticles(0, 0, "GJ_WebSheet", c);
    flyParticleEmitter_->stop();
    flyParticleEmitter_->setDepth(9);
    gameLayer_->container->add(flyParticleEmitter_);
  }
  {
    EmitterConfig c;
    c.speed = Op::R(220, 380); c.angle = Op::R(180, 360); c.lifespan = Op::R(150, 450);
    c.scaleStart = 0.75; c.scaleEnd = 0; c.gravityY = 600; c.frequency = 1000.0 / 30;
    c.blend = Blend::Add; c.tintStart = 16760320; c.tintEnd = 16711680;
    c.alphaStart = 1; c.alphaEnd = 0;
    flyParticle2Emitter_ = s->addParticles(0, 0, "GJ_WebSheet", c);
    flyParticle2Emitter_->stop();
    flyParticle2Emitter_->setDepth(9);
    gameLayer_->container->add(flyParticle2Emitter_);
  }
  {
    EmitterConfig c;
    c.x = Op::R(-18, 18);
    c.speed = Op::R(149.2 * 1.5, 229.2 * 1.5); c.angle = Op::R(205, 295);
    c.lifespan = Op::R(80, 220); c.scaleStart = 0.375; c.scaleEnd = 0;
    c.gravityX = -700; c.gravityY = 600; c.frequency = 25; c.blend = Blend::Add;
    c.alphaStart = 1; c.alphaEnd = 0;
    shipDragEmitter_ = s->addParticles(0, 0, "GJ_WebSheet", c);
    shipDragEmitter_->stop();
    shipDragEmitter_->setDepth(22);
  }
  EmitterConfig land;
  land.speed = Op::R(250, 350); land.angle = Op::R(210, 330); land.lifespan = Op::R(50, 600);
  land.scaleStart = 0.625; land.scaleEnd = 0; land.gravityY = 1000; land.blend = Blend::Add;
  land.alphaStart = 1; land.alphaEnd = 0; land.tintStart = land.tintEnd = colorGreenTint;
  land.emitting = false;
  landEmitter1_ = s->addParticles(0, 0, "GJ_WebSheet", land);
  landEmitter2_ = s->addParticles(0, 0, "GJ_WebSheet", land);
  aboveContainer_ = s->addContainer(0, 0);
  aboveContainer_->setDepth(13);
  aboveContainer_->add(landEmitter1_);
  aboveContainer_->add(landEmitter2_);
  streak_ = std::make_unique<TrailRenderer>(s, 0.231, 10, 8, 100, colorCyanTint, 0.7);
  streak_->addToContainer(gameLayer_->container, 8);
}

void Player::updateParticles(double cameraX, double cameraY, double dt) {
  (void)cameraX;
  if (p->isDead) return;
  double pwx = scene_->playerWorldX, gy = flipY(p->y);
  particleEmitter_->particleX = Op(pwx - 20);
  particleEmitter_->particleY = Op(gy + 26);
  bool onGroundCube = p->onGround && !p->isFlying;
  if (onGroundCube && !particleActive_) { particleEmitter_->start(); particleActive_ = true; }
  else if (!onGroundCube && particleActive_) { particleEmitter_->stop(); particleActive_ = false; }
  {
    double c = std::cos(rotation_), sn = std::sin(rotation_);
    double lx = -24, ly = 18;
    double wx = pwx + lx * c - ly * sn;
    double wy = gy + lx * sn + ly * c;
    double jitter = 2 * (2 * random01() - 1) * 2;
    flyParticleEmitter_->particleX = Op(wx);
    flyParticleEmitter_->particleY = Op(wy + jitter);
    flyParticle2Emitter_->particleX = Op(wx);
    flyParticle2Emitter_->particleY = Op(wy + jitter);
    streak_->setPosition(wx + 8, wy);
  }
  streak_->update(dt);
  bool fly = p->isFlying;
  if (fly && !flyParticleActive_) { flyParticleEmitter_->start(); flyParticleActive_ = true; }
  else if (!fly && flyParticleActive_) { flyParticleEmitter_->stop(); flyParticleActive_ = false; }
  bool boost = p->isFlying && p->upKeyDown;
  if (boost && !flyParticle2Active_) { flyParticle2Emitter_->start(); flyParticle2Active_ = true; }
  else if (!boost && flyParticle2Active_) { flyParticle2Emitter_->stop(); flyParticle2Active_ = false; }
  shipDragEmitter_->x = groundYOffset;
  shipDragEmitter_->particleY = Op(flipY(p->y) + cameraY + 30);
  bool drag = p->isFlying && p->onGround && !p->onCeiling;
  if (drag && !shipDragActive_) { shipDragEmitter_->start(); shipDragActive_ = true; }
  else if (!drag && shipDragActive_) { shipDragEmitter_->stop(); shipDragActive_ = false; }
}

void Player::setCubeVisible(bool v) {
  playerSprite_->setVisible(v);
  if (playerGlow_) playerGlow_->setVisible(v && playerGlow_->glowEnabled);
  if (playerOverlay_) playerOverlay_->setVisible(v);
  if (playerExtra_) playerExtra_->setVisible(v);
}
void Player::setShipVisible(bool v) {
  shipSprite_->setVisible(v);
  if (shipGlow_) shipGlow_->setVisible(v && shipGlow_->glowEnabled);
  if (shipOverlay_) shipOverlay_->setVisible(v);
  if (shipExtra_) shipExtra_->setVisible(v);
}

void Player::syncSprites(double cameraX, double cameraY, double dt, std::optional<double> xo) {
  if (endAnimating_) return;
  double sx = xo ? *xo : groundYOffset;
  double sy = flipY(p->y) + cameraY;
  double rot = rotation_;
  lastCameraX_ = cameraX; lastCameraY_ = cameraY;
  aboveContainer_->x = -cameraX;
  aboveContainer_->y = cameraY;
  if (p->isFlying) {
    const double n = 10;
    double c = std::cos(rot), s = std::sin(rot);
    double ax = -n * s, ay = n * c, bx = n * s, by = -n * c;
    for (GameObject* l : shipLayers_) if (l) { l->x = sx + ax; l->y = sy + ay; l->rotation = rot; }
    for (GameObject* l : playerLayers_) if (l) { l->x = sx + bx; l->y = sy + by; l->rotation = rot; }
  } else {
    for (GameObject* l : allLayers_) if (l) { l->x = sx; l->y = sy; l->rotation = rot; }
  }
  updateParticles(cameraX, cameraY, dt);
}

void Player::enterShipMode(const LevelObject* portal) {
  if (p->isFlying) return;
  p->isFlying = true;
  scene_->toggleGlitter(true);
  p->yVelocity *= 0.5;
  p->onGround = false; p->canJump = false; p->isJumping = false;
  stopRotation();
  rotation_ = 0;
  particleEmitter_->stop();
  flyParticle2Active_ = false;
  streak_->reset();
  streak_->start();
  setShipVisible(true);
  for (GameObject* l : playerLayers_) if (l) l->setScale(0.55);
  double y = p->y;
  if (portal) y = portal->portalY;
  gameLayer_->setFlyMode(true, y);
}

void Player::exitShipMode() {
  if (!p->isFlying) return;
  p->isFlying = false;
  scene_->toggleGlitter(false);
  p->yVelocity *= 0.5;
  p->onGround = false; p->canJump = false; p->isJumping = false;
  stopRotation();
  rotation_ = 0;
  flyParticleEmitter_->stop(); flyParticleActive_ = false;
  flyParticle2Emitter_->stop(); flyParticle2Active_ = false;
  shipDragEmitter_->stop(); shipDragActive_ = false;
  particleActive_ = false;
  streak_->stop();
  streak_->reset();
  setShipVisible(false);
  setCubeVisible(true);
  for (GameObject* l : playerLayers_) if (l) l->setScale(1);
  gameLayer_->setFlyMode(false, 0);
}

void Player::hitGround() {
  bool wasAir = !p->onGround;
  if (!p->isFlying) p->lastGroundY = p->y;
  p->yVelocity = 0;
  p->onGround = true; p->canJump = true; p->isJumping = false;
  stopRotation();
  if (wasAir && !p->isFlying) {
    landIdx_ = !landIdx_;
    ParticleEmitter* e = landIdx_ ? landEmitter1_ : landEmitter2_;
    e->explode(10, lastCameraX_ + groundYOffset, flipY(p->y) + 30);
  }
}

void Player::killPlayer() {
  if (p->isDead) return;
  p->isDead = true;
  scene_->toggleGlitter(false);
  particleEmitter_->stop(); particleActive_ = false;
  flyParticleEmitter_->stop(); flyParticleActive_ = false;
  flyParticle2Emitter_->stop(); flyParticle2Active_ = false;
  shipDragEmitter_->stop(); shipDragActive_ = false;
  streak_->stop();
  streak_->reset();
  Scene* s = scene_;
  double ex = scene_->playerWorldX - scene_->cameraX;
  double ey = flipY(p->y) + lastCameraY_;
  const double k = 0.9;
  EmitterConfig c;
  c.speed = Op::R(200, 800); c.angle = Op::R(0, 360);
  c.scaleStart = 18.0 / 32; c.scaleEnd = 0; c.alphaStart = 1; c.alphaEnd = 0;
  c.lifespan = Op::R(50, 800); c.quantity = 100; c.stopAfter = 100; c.blend = Blend::Add;
  c.tintStart = c.tintEnd = colorGreenTint;
  c.x = Op::R(-20, 20); c.y = Op::R(-20, 20);
  ParticleEmitter* burst = s->addParticles(ex, ey, "GJ_WebSheet", c);
  burst->setDepth(15);
  Graphics* g = s->addGraphics();
  g->setDepth(15)->setBlendMode(Blend::Add);
  s->tween(g, 500, ease::quadOut,
           [g, ex, ey](double t) {
             g->clear();
             g->fillStyle(colorGreenTint, 1 - t);
             g->fillCircle(ex, ey, 18 + 144 * t);
           },
           [g]() { g->destroy(); });
  createExplosionPieces(ex, ey, k);
  setCubeVisible(false);
  setShipVisible(false);
}

void Player::createExplosionPieces(double x, double y, double extra) {
  Scene* scene = scene_;
  double half = 40 * extra;
  int size = (int)jsRound(2 * half);
  Renderer& r = scene->app->renderer;
  SDL_Texture* rt = SDL_CreateTexture(r.sdl(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                      size, size);
  SDL_SetTextureBlendMode(rt, SDL_BLENDMODE_BLEND);
  r.setOffset(0, 0);
  r.setTarget(rt, true);
  GameObject* order[] = {playerGlow_, playerOverlay_, shipGlow_, shipOverlay_,
                         playerSprite_, playerExtra_, shipSprite_, shipExtra_};
  for (GameObject* l : order) {
    if (!l || !l->visible) continue;
    Mat m = Mat::TRS(size / 2.0 + (l->x - x), size / 2.0 + (l->y - y), l->rotation, l->scaleX, l->scaleY);
    l->render(r, m, l->alpha);
  }
  r.setTarget(nullptr, false);

  static int counter = 0;
  explosionTexKey_ = "__deathRT_" + std::to_string(counter++);
  Texture* tex = scene->app->textures->addTexture(explosionTexKey_, rt, size, size);

  int nx = 2 + (int)jsRound(2 * random01());
  int ny = 2 + (int)jsRound(2 * random01());
  double roll = random01();
  if (roll > 0.95) nx = 1;
  else if (roll > 0.9) ny = 1;
  const double base = 9.34740324 * 0.8, xv = 0.5 * base, xr = 1 * base, jitter = 0.45;
  double cw = (double)size / nx, ch = (double)size / ny;
  std::vector<double> widths, heights, xs{0}, ys{0};
  double accX = 0, accY = 0;
  for (int i = 0; i < nx - 1; i++) {
    double w = jsRound(cw * (0.55 + random01() * jitter * 2));
    widths.push_back(w); accX += w; xs.push_back(accX);
  }
  widths.push_back(size - accX);
  for (int i = 0; i < ny - 1; i++) {
    double h = jsRound(ch * (0.55 + random01() * jitter * 2));
    heights.push_back(h); accY += h; ys.push_back(accY);
  }
  heights.push_back(size - accY);

  pieces_.clear();
  hasPieces_ = true;
  explosionContainer_ = scene->addContainer(x, y);
  explosionContainer_->setDepth(16);
  int count = 0;
  for (int i = 0; i < nx; i++) {
    double w = widths[i], x0 = xs[i];
    for (int j = 0; j < ny; j++) {
      double h = heights[j], y0 = ys[j];
      if (w <= 0 || h <= 0) continue;
      count++;
      std::string name = "piece_" + std::to_string(i) + "_" + std::to_string(j);
      tex->add(name, (int)x0, (int)y0, (int)w, (int)h);
      Image* img = scene->addImage(0, 0, explosionTexKey_, name);
      img->x = x0 + w / 2 - size / 2.0;
      img->y = -(y0 + h / 2 - size / 2.0);
      explosionContainer_->add(img);
      ParticleEmitter* trail = nullptr;
      if (count % 2 == 0) {
        double life = 200 + 200 * random01();
        EmitterConfig c;
        c.speed = Op(0); c.scaleStart = 0.5; c.scaleEnd = 0; c.alphaStart = 1; c.alphaEnd = 0;
        c.lifespan = Op(life); c.frequency = 25; c.quantity = 1; c.emitting = true;
        c.blend = Blend::Add; c.tintStart = c.tintEnd = colorGreenTint;
        c.emitCallback = [img](Particle& pt) {
          pt.x = img->x + 3 * (2 * random01() - 1) * 2;
          pt.y = img->y + 3 * (2 * random01() - 1) * 2;
        };
        trail = scene->addParticles(0, 0, "GJ_WebSheet", c);
        explosionContainer_->addAt(trail, 0);
      }
      Piece pc;
      pc.spr = img; pc.particle = trail;
      pc.xVel = xv + (2 * random01() - 1) * xr;
      pc.yVel = -(12 + 6 * (2 * random01() - 1));
      pc.timer = 1.4; pc.fadeTime = 0.5;
      pc.rotDelta = (360 * (2 * random01() - 1)) / 60;
      pc.halfSize = std::min(w, h) / 2;
      pieces_.push_back(pc);
    }
  }
  explosionGroundSY_ = flipY(0) + lastCameraY_;
}

void Player::updateExplosionPieces(double deltaMs) {
  if (!hasPieces_ || pieces_.empty()) return;
  double dt = deltaMs / 1000.0;
  double f = std::min(60 * dt * 0.9, 2.0);
  double grav = 0.5 * f * 2;
  double groundRel = explosionGroundSY_ - explosionContainer_->y;
  for (size_t i = 0; i < pieces_.size();) {
    Piece& pc = pieces_[i];
    pc.timer -= dt;
    if (pc.timer > 0) {
      pc.yVel += grav;
      pc.xVel *= 0.98 + 0.02 * (1 - f);
      double nx = pc.spr->x + pc.xVel * f;
      double ny = pc.spr->y + pc.yVel * f;
      double floorY = groundRel - pc.halfSize;
      if (ny > floorY && pc.yVel > 0) {
        ny = floorY;
        pc.yVel *= -0.8;
        if (std::fabs(pc.yVel) < 3) pc.yVel = -3;
      }
      pc.spr->x = nx; pc.spr->y = ny;
      pc.spr->setAngle(pc.spr->angle() + pc.rotDelta * f);
      if (pc.timer < pc.fadeTime) {
        double a = pc.timer / pc.fadeTime;
        pc.spr->setAlpha(a);
        if (pc.particle) pc.particle->setAlpha(a);
      }
      i++;
    } else {
      if (pc.particle) { pc.particle->stop(); pc.particle->destroy(); }
      pc.spr->destroy();
      pieces_.erase(pieces_.begin() + i);
    }
  }
  if (pieces_.empty()) cleanupExplosion();
}

void Player::cleanupExplosion() {
  if (hasPieces_) {
    for (Piece& pc : pieces_) {
      if (pc.particle) { pc.particle->stop(); pc.particle->destroy(); }
      if (pc.spr) pc.spr->destroy();
    }
  }
  if (explosionContainer_) { explosionContainer_->destroy(); explosionContainer_ = nullptr; }
  if (!explosionTexKey_.empty()) {
    scene_->app->textures->remove(explosionTexKey_);
    explosionTexKey_.clear();
  }
  pieces_.clear();
  hasPieces_ = false;
}

void Player::playPortalShine(const LevelObject* pt) {
  Scene* scene = scene_;
  double x = pt->x, y = flipY(pt->portalY);
  const char* names[2] = {"portalshine_02_front_001.png", "portalshine_02_back_001.png"};
  Container* targets[2] = {gameLayer_->topContainer, gameLayer_->container};
  for (int i = 0; i < 2; i++) {
    AtlasRef ref = findAtlasFrame(scene, names[i]);
    if (!ref.ok) continue;
    Image* im = scene->addImage(x, y, ref.atlas, ref.frame);
    im->setBlendMode(Blend::Add);
    im->setAlpha(0);
    targets[i]->add(im);
    scene->tween(im, 50, ease::linear, [im](double t) { im->alpha = t; },
                 [scene, im]() {
                   auto from = std::make_shared<double>(1);
                   scene->tween(im, 400, ease::linear,
                                [im, from](double t) { im->alpha = *from * (1 - t); },
                                [im]() { im->destroy(); }, 0, false, 0,
                                [im, from]() { *from = im->alpha; });
                 });
  }
}

void Player::checkSnapJump(LevelObject* pt) {
  static const double table[3][2] = {{240, 60}, {300, -60}, {180, 120}};
  if (lastLandObject_ && lastLandObject_ != pt && lastLandObject_->type == ObjKind::Solid) {
    double lx = lastLandObject_->x, ly = lastLandObject_->y, nx = pt->x, ny = pt->y;
    double fm = p->gravityFlipped ? -1 : 1;
    bool snap = false;
    for (auto& e : table) {
      if (std::fabs(nx - (lx + e[0])) <= 2 && std::fabs(ny - (ly + e[1] * fm)) <= 2) { snap = true; break; }
    }
    if (snap) {
      double target = pt->x + lastXOffset_;
      double pwx = scene_->playerWorldX, v;
      if (std::fabs(target - pwx) <= 2) v = target;
      else v = target > pwx ? pwx + 2 : pwx - 2;
      scene_->playerWorldX = v;
    }
  }
  lastLandObject_ = pt;
  lastXOffset_ = scene_->playerWorldX - pt->x;
}

bool Player::isFallingPastThreshold() const {
  return p->gravityFlipped ? p->yVelocity > 0.25 : p->yVelocity < -0.25;
}

void Player::runRotateAction() {
  rotateActionActive = true;
  rotateActionTime_ = 0;
  rotateActionDuration_ = 0.39 / physicsConst09;
  rotateActionStart_ = rotation_;
  rotateActionTotal_ = PI * flipMod();
}
void Player::stopRotation() { rotateActionActive = false; }
void Player::updateRotateAction(double dt) {
  if (!rotateActionActive) return;
  rotateActionTime_ += dt;
  if (rotateActionTime_ >= rotateActionDuration_) rotateActionActive = false;
  double t = std::min(rotateActionTime_ / rotateActionDuration_, 1.0);
  rotation_ = rotateActionStart_ + rotateActionTotal_ * t;
}
double Player::convertToClosestRotation() const {
  double q = PI / 2;
  return jsRound(rotation_ / q) * q;
}
double Player::slerp2D(double a, double b, double t) {
  double d = b - a;
  while (d > PI) d -= 2 * PI;
  while (d < -PI) d += 2 * PI;
  return a + d * t;
}
void Player::updateGroundRotation(double dt) {
  double target = convertToClosestRotation();
  double rate = 0.1575 * 3;
  double t = std::min(1 * dt, rate * dt);
  rotation_ = slerp2D(rotation_, target, t);
}
void Player::updateShipRotation(double dt) {
  double dy = -(p->y - p->lastY);
  double dx = 10.3860036 * dt;
  if (dx * dx + dy * dy >= 0.6 * dt) {
    double target = std::atan2(dy, dx);
    double rate = 0.15;
    double t = std::min(1 * dt, rate * dt);
    rotation_ = slerp2D(rotation_, target, t);
  }
}
bool Player::playerIsFalling() const {
  return p->gravityFlipped ? p->yVelocity > 3.832796 : p->yVelocity < 3.832796;
}

void Player::updateJump(double dt) {
  if (p->isFlying) { updateFlyJump(dt); return; }
  if (p->upKeyDown && p->canJump) {
    p->isJumping = true; p->onGround = false; p->canJump = false; p->upKeyPressed = false;
    p->yVelocity = 22.360064 * flipMod();
    runRotateAction();
  } else if (p->isJumping) {
    p->yVelocity -= physicsConst1916 * dt * flipMod();
    if (playerIsFalling()) { p->isJumping = false; p->onGround = false; }
  } else {
    if (playerIsFalling()) p->canJump = false;
    p->yVelocity -= physicsConst1916 * dt * flipMod();
    if (p->gravityFlipped) p->yVelocity = std::min(p->yVelocity, 30.0);
    else p->yVelocity = std::max(p->yVelocity, -30.0);
    if (isFallingPastThreshold() && !rotateActionActive) runRotateAction();
    if (playerIsFalling()) {
      bool fast = p->gravityFlipped ? p->yVelocity > 4 : p->yVelocity < -4;
      if (fast) p->onGround = false;
    }
  }
}

void Player::updateFlyJump(double dt) {
  double a = 0.8;
  if (p->upKeyDown && !p->wasBoosted) a = -1;
  if (!(p->upKeyDown || playerIsFalling())) a = 1.2;
  double b = 0.4;
  if (p->upKeyDown && playerIsFalling()) b = 0.5;
  p->yVelocity -= physicsConst1916 * dt * flipMod() * a * b;
  if (p->upKeyDown) p->onGround = false;
  if (!p->wasBoosted) {
    if (p->gravityFlipped) {
      p->yVelocity = std::max(p->yVelocity, -16.0);
      p->yVelocity = std::min(p->yVelocity, 12.8);
    } else {
      p->yVelocity = std::max(p->yVelocity, -12.8);
      p->yVelocity = std::min(p->yVelocity, 16.0);
    }
  }
}

void Player::checkCollisions(double cameraX) {
  const double n30 = 30;
  double px_ = cameraX + groundYOffset;
  double py = p->y, lastY = p->lastY;
  double slack = p->isFlying ? 12 : 20;
  p->collideTop = 0; p->collideBottom = 0; p->onCeiling = false;
  bool landed = false;
  const auto& nearby = gameLayer_->getNearbySectionObjects(px_);
  // copy: the buffer is reused by LevelRenderer
  std::vector<LevelObject*> items(nearby.begin(), nearby.end());
  for (LevelObject* item : items) {
    double l = item->x - item->w / 2, r = item->x + item->w / 2;
    double b = item->y - item->h / 2, t = item->y + item->h / 2;
    if (px_ + 30 <= l || px_ - 30 >= r || py + n30 <= b || py - n30 >= t) continue;
    if (item->type != ObjKind::PortalFly) {
      if (item->type != ObjKind::PortalCube) {
        if (item->type == ObjKind::Hazard) { killPlayer(); return; }
        if (item->type == ObjKind::Solid) {
          double v83 = py - n30 + slack, v84 = lastY - n30 + slack;
          double v85 = py + n30 - slack, v86 = lastY + n30 - slack;
          const double n9 = 9;
          bool inner = px_ + n9 > l && px_ - n9 < r && py + n9 > b && py - n9 < t;
          bool standing = (p->yVelocity <= 0 || p->onGround) && (v83 >= t || v84 >= t);
          if (inner && !standing) { killPlayer(); return; }
          if (px_ + 30 - 5 > l && px_ - 30 + 5 < r) {
            if ((v83 >= t || v84 >= t) && (p->yVelocity <= 0 || p->onGround)) {
              p->y = t + n30;
              hitGround();
              landed = true;
              p->collideBottom = t;
              if (!p->isFlying) checkSnapJump(item);
              continue;
            }
            if ((v85 <= b || v86 <= b) && (p->yVelocity >= 0 || p->onGround) && p->isFlying) {
              p->y = b - n30;
              hitGround();
              p->onCeiling = true;
              p->collideTop = b;
              continue;
            }
          }
        }
      } else if (!item->activated) {
        item->activated = true;
        playPortalShine(item);
        exitShipMode();
      }
    } else if (!item->activated) {
      item->activated = true;
      playPortalShine(item);
      enterShipMode(item);
    }
  }
  if (p->collideTop != 0 && p->collideBottom != 0)
    if (std::fabs(p->collideTop - p->collideBottom) < 48) { killPlayer(); return; }
  double floorY = gameLayer_->getFloorY();
  if (!landed && p->y <= floorY + 30) { p->y = floorY + 30; hitGround(); }
  std::optional<double> ceilY = gameLayer_->getCeilingY();
  if (ceilY && p->y >= *ceilY - 30) { p->y = *ceilY - 30; hitGround(); p->onCeiling = true; }
  if (p->isFlying) {
    bool atFloor = p->y <= floorY + 30;
    bool atCeil = ceilY && p->y >= *ceilY - 30;
    if (!landed && !atFloor && p->collideTop == 0 && !atCeil) p->onGround = false;
  }
}

void Player::drawHitboxes(Graphics* g, double cameraX, double cameraY) {
  g->clear();
  if (!showHitboxes_) return;
  const double n = 30;
  double px_ = cameraX + groundYOffset, py = p->y;
  double slack = p->isFlying ? 12 : 20;
  const auto& nearby = gameLayer_->getNearbySectionObjects(px_);
  for (LevelObject* item : nearby) {
    double sx = item->x - cameraX, sy = flipY(item->y) + cameraY;
    uint32_t col = 65280;
    if (item->type == ObjKind::Hazard) col = 16729156;
    else if (item->type == ObjKind::PortalFly || item->type == ObjKind::PortalCube) col = 4491519;
    g->lineStyle(2, col, 0.7);
    g->strokeRect(sx - item->w / 2, sy - item->h / 2, item->w, item->h);
  }
  double bx = groundYOffset, by = flipY(py) + cameraY;
  g->lineStyle(2, 65535, 0.8);
  g->strokeRect(bx - n, by - n, baseUnit, baseUnit);
  g->lineStyle(2, 16776960, 0.8);
  g->strokeRect(bx - n + 5, by - n, 50, baseUnit);
  g->lineStyle(2, 16711680, 0.8);
  g->strokeRect(bx - n, by - n + 5, baseUnit, 50);
  double y1 = flipY(py - n + slack) + cameraY, y2 = flipY(py + n - slack) + cameraY;
  g->lineStyle(2, 16746496, 0.9);
  g->lineBetween(bx - n - 8, y1, bx + n + 8, y1);
  g->lineBetween(bx - n - 8, y2, bx + n + 8, y2);
  g->lineStyle(2, 16777215, 1);
  g->strokeRect(bx - 9, by - 9, 36, 18);
}

void Player::playEndAnimation(double targetX, std::function<void()> done, double portalY) {
  endAnimating_ = true;
  Scene* scene = scene_;
  double endY = portalY != 0 ? portalY : 240;
  double pwx = scene_->playerWorldX, py = p->y;
  double x3 = targetX + 100, y3 = endY - 40;
  double x0 = pwx, y0 = py, x2 = pwx + 80, y2 = endY + 300;
  GameObject* cand[] = {playerSprite_, playerGlow_, playerOverlay_, playerExtra_,
                        shipSprite_, shipGlow_, shipOverlay_, shipExtra_};
  std::vector<GameObject*> sprites;
  for (GameObject* l : cand) if (l && l->visible) sprites.push_back(l);
  particleEmitter_->stop(); flyParticleEmitter_->stop();
  flyParticle2Emitter_->stop(); shipDragEmitter_->stop();
  bool fly = p->isFlying;
  struct Item { GameObject* spr; double localY; };
  auto items = std::make_shared<std::vector<Item>>();
  for (GameObject* s : sprites) {
    double ly = 0;
    if (fly) {
      bool isShip = s == shipSprite_ || s == shipGlow_ || s == shipOverlay_ || s == shipExtra_;
      bool isPlayer = s == playerSprite_ || s == playerGlow_ || s == playerOverlay_ || s == playerExtra_;
      if (isShip) ly = 10; else if (isPlayer) ly = -10;
    }
    items->push_back({s, ly});
  }
  TrailRenderer* streak = streak_.get();
  GameScene* gs = scene_;
  if (!items->empty())
    scene->tween(this, 1000, [](double v) { return std::pow(v, 1.2); },
      [=](double val) {
        double u = 1 - val;
        double bxp = u * u * u * x0 + 3 * u * u * val * x0 + 3 * u * val * val * x2 + val * val * val * x3;
        double byp = u * u * u * y0 + 3 * u * u * val * y0 + 3 * u * val * val * y2 + val * val * val * y3;
        double sx = bxp - gs->cameraX;
        double sy = flipY(byp) + gs->cameraY;
        double alpha = 1 - val * val;
        double rot = (*items)[0].spr->rotation;
        double c = std::cos(rot), s = std::sin(rot);
        for (auto& it : *items) {
          it.spr->setPosition(sx + (-it.localY * s), sy + it.localY * c);
          it.spr->setAlpha(alpha);
        }
        streak->setPosition(bxp, flipY(byp));
        streak->update(gs->app->loopDelta / 1000.0);
      },
      [=]() {
        for (auto& it : *items) it.spr->setVisible(false);
        streak->stop(); streak->reset();
        done();
      });
  for (GameObject* s : sprites) {
    auto from = std::make_shared<double>(0);
    double to = s->angle() + 360;
    scene->tween(s, 1000, [](double v) { return std::pow(v, 1.5); },
                 [s, from, to](double t) { s->setAngle(*from + (to - *from) * t); }, nullptr, 0,
                 false, 0, [s, from]() { *from = s->angle(); });
  }
}

void Player::reset() {
  cleanupExplosion();
  endAnimating_ = false;
  lastLandObject_ = nullptr;
  lastXOffset_ = 0;
  stopRotation();
  rotateActionTime_ = 0;
  rotation_ = 0;
  lastCameraX_ = 0; lastCameraY_ = 0;
  setCubeVisible(true);
  setShipVisible(false);
  for (GameObject* l : allLayers_) if (l) l->setAlpha(1);
  for (GameObject* l : playerLayers_) if (l) l->setScale(1);
  particleEmitter_->stop(); particleActive_ = false;
  flyParticleEmitter_->stop(); flyParticleActive_ = false;
  flyParticle2Emitter_->stop(); flyParticle2Active_ = false;
  shipDragEmitter_->stop(); shipDragActive_ = false;
  streak_->stop();
  streak_->reset();
}
