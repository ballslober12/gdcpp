// Port of game-scene.js
#include "game_scene.h"
#include <algorithm>
#include <cmath>
#include "win_effects.h"

using namespace px;

namespace {
const char* kFsOn = "toggleFullscreenOn_001.png";
const char* kFsOff = "toggleFullscreenOff_001.png";
Frame* frameOf(Scene* s, const std::string& name) {
  Texture* t = s->app->textures->get("GJ_WebSheet");
  return t ? t->get(name) : nullptr;
}
void setAtlasFrame(Scene* s, Image* im, const std::string& name) {
  Texture* t = s->app->textures->get("GJ_WebSheet");
  Frame* f = t ? t->get(name) : nullptr;
  if (f) im->setTexture(t, f);
}
}  // namespace

GameScene::GameScene(App* a) : Scene(a) {}
GameScene::~GameScene() {}

void GameScene::create() {
  app->onScreenWidthChanged = [](double w) { setScreenWidth(w); };
  setScreenWidth(app->screenW);
  bgSpeedX_ = 0.1; bgSpeedY_ = 0.1;
  menuCameraX_ = -groundYOffset;
  prevCameraX_ = -groundYOffset;
  bg_ = addTileSprite(0, 0, screenWidth, screenHeight, "game_bg_01");
  bg_->setOrigin(0, 0)->setDepth(-10);
  double height = app->textures->get("game_bg_01")->h;
  bgInitY_ = height - screenHeight - unusedConst180;
  cameraX = -groundYOffset;
  cameraY = 0;
  cameraXRef_.value = -groundYOffset;
  state_ = std::make_unique<PlayerPhysicsState>();
  level_ = std::make_unique<LevelRenderer>(this, &cameraXRef_);
  level_->cameraYRef = &cameraY;
  player_ = std::make_unique<Player>(this, state_.get(), level_.get());
  colorManager_ = std::make_unique<ColorManager>();
  audio_ = std::make_unique<AudioManager>(this, &sfxVolume_);
  if (!app->levelText.empty()) level_->loadLevel(app->levelText);
  level_->createEndPortal(this);
  glitterCenterX_ = 0; glitterCenterY_ = yFlipBase;
  {
    EmitterConfig c;
    c.frame = "square.png"; c.speed = Op(0);
    c.scaleStart = 0.375; c.scaleEnd = 0; c.alphaStart = 1; c.alphaEnd = 0;
    c.lifespan = Op::R(200, 1800); c.frequency = 60; c.blend = Blend::Add;
    c.tintStart = c.tintEnd = colorGreenTint; c.emitting = false;
    c.emitCallback = [this](Particle& pt) {
      pt.x = glitterCenterX_ + (2 * random01() - 1) * (screenWidth / 1.8);
      pt.y = glitterCenterY_ + 320 * (2 * random01() - 1);
    };
    glitterEmitter_ = addParticles(0, 0, "GJ_WebSheet", c);
    level_->additiveContainer->add(glitterEmitter_);
  }
  bg_->setTint(colorManager_->getHex(num30));
  level_->setGroundColor(colorManager_->getHex(num31));
  level_->additiveContainer->setVisible(false);
  level_->container->setVisible(false);
  level_->topContainer->setVisible(false);
  attempts_ = 1; bestPercent_ = 0; lastPercent_ = 0;
  endPortalGameY_ = 240;
  resetGameplayState();
  totalJumps_ = 0; playTime_ = 0;
  menuActive_ = true; slideIn_ = false; slideGroundX_.reset(); firstPlay_ = true;
  player_->setCubeVisible(false);
  player_->setShipVisible(false);
  logo_ = addImage(0, 100, "GJ_WebSheet", "GJ_logo_001.png");
  logo_->setDepth(30);
  robLogo_ = addImage(160, 555, "GJ_WebSheet", "RobTopLogoBig_001.png");
  robLogo_->setDepth(30)->setScale(0.9);
  copyrightText_ = addText(0, 625, "\xC2\xA9 2026 RobTop Games \xC2\xB7 geometrydash.com", 14, 0xffffff);
  copyrightText_->setOrigin(1, 1)->setDepth(30)->setAlpha(0.3);
  tryMeImg_ = addImage(0, 182.5, "GJ_WebSheet", "tryMe_001.png");
  tryMeImg_->setDepth(30);
  struct DL { const char* key; const char* url; };
  const DL table4[] = {
      {"downloadSteam_001", "https://store.steampowered.com/app/322170/Geometry_Dash"},
      {"downloadGoogle_001", "https://play.google.com/store/apps/details?id=com.robtopx.geometryjump&hl=en"},
      {"downloadApple_001", "https://apps.apple.com/us/app/geometry-dash/id625334537"}};
  for (const DL& d : table4) {
    double s = 1 / 1.5;
    Image* im = addImage(0, 0, "GJ_WebSheet", std::string(d.key) + ".png");
    im->setDepth(30)->setScale(s);
    im->setInteractive();
    std::string url = d.url;
    makeBouncyButton(im, s, [url]() { SDL_OpenURL(url.c_str()); }, [this]() { return menuActive_; });
    downloadBtns_.push_back(im);
  }
  const uint32_t blueTint = 0x0066ff;
  menuFsBtn_ = addImage(33, 33, "GJ_WebSheet", app->fullscreen ? kFsOff : kFsOn);
  menuFsBtn_->setDepth(30)->setScale(0.64)->setAlpha(0.8)->setTint(blueTint);
  menuFsBtn_->setInteractive();
  expandHitArea(menuFsBtn_, 1.5);
  makeBouncyButton(menuFsBtn_, 0.64,
                   [this]() {
                     bool v = !app->fullscreen;
                     setAtlasFrame(this, menuFsBtn_, v ? kFsOff : kFsOn);
                     expandHitArea(menuFsBtn_, 1.5);
                     toggleFullscreen();
                   },
                   [this]() { return menuActive_; });
  menuInfoBtn_ = addImage(screenWidth - 30 - 3, 33, "GJ_WebSheet", "GJ_infoIcon_001.png");
  menuInfoBtn_->setDepth(30)->setScale(0.64)->setAlpha(0.8)->setTint(blueTint);
  menuInfoBtn_->setInteractive();
  expandHitArea(menuInfoBtn_, 1.5);
  makeBouncyButton(menuInfoBtn_, 0.64, [this]() { buildInfoPopup(); },
                   [this]() { return menuActive_ && !infoPopup_; });
  {
    EmitterConfig c;
    c.frame = "square.png"; c.speed = Op(0);
    c.scaleStart = 0.5; c.scaleEnd = 0; c.alphaStart = 0.6; c.alphaEnd = 0.2;
    c.lifespan = Op::R(1000, 2000); c.frequency = 35; c.blend = Blend::Add;
    c.tintStart = c.tintEnd = 20670; c.x = Op::R(-130, 130); c.y = Op::R(-100, 100);
    menuGlitter_ = addParticles(0, 0, "GJ_WebSheet", c);
    menuGlitter_->setDepth(29);
  }
  playBtn_ = addImage(0, 0, "GJ_WebSheet", "GJ_playBtn_001.png");
  playBtn_->setDepth(30);
  playBtn_->setInteractive();
  playBtnPressed_ = false;
  makeBouncyButton(playBtn_, 1,
                   [this]() {
                     audio_->playEffect("playSound_01", 1);
                     startGame();
                   },
                   [this]() { return menuActive_ && !playBtnPressed_; });
  positionMenuItems();
  spaceWasDown_ = false;
  pauseBtn_ = addImage(screenWidth - 30, 30, "GJ_WebSheet", "GJ_pauseBtn_clean_001.png");
  pauseBtn_->setDepth(30)->setAlpha(75.0 / 255)->setVisible(false);
  pauseBtn_->setInteractive();
  expandHitArea(pauseBtn_, 2);
  pauseBtn_->input->onDown = [this]() { pauseGame(); };
  paused_ = false;
  pauseContainer_ = nullptr;
  {
    auto it = app->registry.find("userSfxVol");
    sfxVolume_ = it != app->registry.end() ? it->second : 1;
  }
  onKeyDown = [this](SDL_Keycode k) {
    if (k == SDLK_ESCAPE) {
      if (paused_) resumeGame();
      else if (!(menuActive_ || slideIn_ || state_->isDead || levelWon_)) pauseGame();
    } else if (k == SDLK_h) {
      fpsText_->setVisible(!fpsText_->visible);
    }
  };
  onPointerDown = [this]() { if (!(menuActive_ || paused_)) pushButton(); };
  onPointerUp = [this]() {
    if (!(menuActive_ || paused_)) releaseButton();
    releaseButton();  // window pointerup / touchend listeners
  };
  buildHUD();
  auto fi = app->registry.find("fadeInFromBlack");
  if (fi != app->registry.end() && fi->second) {
    app->registry.erase(fi);
    cameraFadeIn(400);
  }
}

void GameScene::onSuspend(bool hidden) {
  if (hidden) audio_->pauseMusic();
  else if (!(menuActive_ || paused_ || state_->isDead || levelWon_)) audio_->resumeMusic();
}

void GameScene::buildHUD() {
  attemptsLabel_ = addBitmapText(0, 0, "bigFont", "Attempt 1", 65);
  attemptsLabel_->setOrigin(0.5, 0.5)->setVisible(false);
  level_->topContainer->add(attemptsLabel_);
  positionAttemptsLabel();
  fpsText_ = addText(screenWidth - 20, 10, "", 28, 0xff0000);
  fpsText_->setOrigin(1, 0)->setDepth(999)->setVisible(false);
  fpsAccum_ = 0; fpsFrames_ = 0;
}

void GameScene::toggleGlitter(bool v) {
  if (v) glitterEmitter_->start();
  else glitterEmitter_->stop();
}

void GameScene::setParticleTimeScale(double v) {
  std::function<void(GameObject*)> rec = [&](GameObject* o) {
    if (!o) return;
    if (auto* e = dynamic_cast<ParticleEmitter*>(o)) e->timeScale = v;
    if (auto* c = dynamic_cast<Container*>(o)) for (GameObject* k : c->list) rec(k);
  };
  rec(level_->container);
  rec(level_->topContainer);
  if (glitterEmitter_) glitterEmitter_->timeScale = v;
}

void GameScene::pauseGame() {
  if (paused_ || menuActive_ || slideIn_ || state_->isDead || levelWon_) return;
  paused_ = true;
  pauseBtn_->setVisible(false);
  audio_->pauseMusic();
  setParticleTimeScale(0);
  buildPauseOverlay();
}

void GameScene::resumeGame() {
  if (!paused_) return;
  setParticleTimeScale(1);
  paused_ = false;
  pauseBtn_->setVisible(true)->setAlpha(75.0 / 255);
  audio_->resumeMusic();
  if (pauseContainer_) { pauseContainer_->destroy(); pauseContainer_ = nullptr; }
}

void GameScene::buildPauseOverlay() {
  double cx = screenWidth / 2, cy = 320, panelW = screenWidth - 40;
  pauseContainer_ = addContainer(0, 0);
  pauseContainer_->setDepth(100);
  Rectangle* rect = addRectangle(cx, cy, screenWidth, screenHeight, 0, 75.0 / 255);
  rect->setInteractive();
  pauseContainer_->add(rect);
  double corner = 0.325 * app->textures->get("square04_001")->w;
  Container* panel = drawScale9(cx, cy, panelW, 600, "square04_001", corner, 0, 150.0 / 255);
  pauseContainer_->add(panel);
  Image* fsImg = addImage(cx - panelW / 2 + 40, 60, "GJ_WebSheet", app->fullscreen ? kFsOff : kFsOn);
  fsImg->setScale(0.64);
  fsImg->setInteractive();
  expandHitArea(fsImg, 2.5);
  pauseContainer_->add(fsImg);
  makeBouncyButton(fsImg, 0.64, [this, fsImg]() {
    bool v = !app->fullscreen;
    setAtlasFrame(this, fsImg, v ? kFsOff : kFsOn);
    expandHitArea(fsImg, 2.5);
    toggleFullscreen();
  });
  pauseContainer_->add(addBitmapText(cx, 65, "bigFont", "Stereo Madness", 40)->setOrigin(0.5, 0.5));
  double barY = 170;
  int best = (int)bestPercent_;
  Image* barBg = addImage(cx, barY, "GJ_WebSheet", "GJ_progressBar_001.png");
  barBg->setTint(0)->setAlpha(125.0 / 255);
  pauseContainer_->add(barBg);
  Frame* bf = frameOf(this, "GJ_progressBar_001.png");
  double bw = bf ? bf->width : 680, bh = bf ? bf->height : 40;
  double cropW = std::max(1.0, std::floor(bw * (best / 100.0)));
  Image* bar = addImage(0, 0, "GJ_WebSheet", "GJ_progressBar_001.png");
  bar->setTint(65280)->setScale(0.992, 0.86)->setOrigin(0, 0.5);
  bar->setCrop(0, 0, cropW, bh);
  bar->setPosition(cx - (0.992 * bw) / 2, barY);
  pauseContainer_->add(bar);
  pauseContainer_->add(addBitmapText(cx, barY, "bigFont", std::to_string(best) + "%", 30)
                           ->setOrigin(0.5, 0.5)->setScale(0.7));
  pauseContainer_->add(addBitmapText(cx, 130, "bigFont", "Normal Mode", 30)
                           ->setOrigin(0.5, 0.5)->setScale(0.78));
  struct Btn { const char* frame; std::function<void()> action; };
  std::vector<Btn> btns = {
      {"GJ_replayBtn_001.png", [this]() { resumeGame(); restartLevel(); }},
      {"GJ_playBtn2_001.png", [this]() { resumeGame(); }},
      {"GJ_menuBtn_001.png", [this]() {
         audio_->playEffect("quitSound_01");
         audio_->stopMusic();
         resumeGame();
         app->requestSceneRestart();
       }}};
  std::vector<double> widths;
  double total = 0;
  for (Btn& b : btns) {
    Frame* f = frameOf(this, b.frame);
    widths.push_back(f ? f->width : 246);
    total += widths.back();
  }
  double x = cx - (total + 40 * (btns.size() - 1)) / 2;
  for (size_t i = 0; i < btns.size(); i++) {
    Image* im = addImage(x + widths[i] / 2, 330, "GJ_WebSheet", btns[i].frame);
    im->setInteractive();
    pauseContainer_->add(im);
    makeBouncyButton(im, 1, btns[i].action);
    x += widths[i] + 40;
  }
  const double sy = 500, sc = 0.7;
  Frame* gf = frameOf(this, "slidergroove.png");
  double gw = gf ? gf->width : 420;
  auto slider = [&](double sx, const char* icon, double initial, std::function<void(double)> onChange) {
    pauseContainer_->add(addImage(sx - 180 - 5, sy, "GJ_WebSheet", icon)->setScale(1.2));
    double span = (gw - 8) * sc;
    double left = sx - (gw * sc) / 2 + 2.8;
    double fillW = initial * span;
    TileSprite* tile = addTileSprite(left, sy, fillW > 0 ? fillW : 1, 11.2, "sliderBar");
    tile->setOrigin(0, 0.5)->setVisible(fillW > 0);
    pauseContainer_->add(tile);
    pauseContainer_->add(addImage(sx, sy, "GJ_WebSheet", "slidergroove.png")->setScale(sc));
    Image* thumb = addImage(left + initial * span, sy, "GJ_WebSheet", "sliderthumb.png");
    thumb->setScale(sc);
    Interactive* in = thumb->setInteractive();
    in->draggable = true;
    pauseContainer_->add(thumb);
    in->onDown = [this, thumb]() { setAtlasFrame(this, thumb, "sliderthumbsel.png"); };
    in->onUp = [this, thumb]() { setAtlasFrame(this, thumb, "sliderthumb.png"); };
    in->onOut = [this, thumb]() { setAtlasFrame(this, thumb, "sliderthumb.png"); };
    in->onDrag = [thumb, tile, left, span, onChange](double dx, double) {
      thumb->x = std::max(left, std::min(left + span, dx));
      double v = (thumb->x - left) / span;
      if (v < 0.03) v = 0;
      tile->w = std::max(1.0, v * span);
      tile->setVisible(v > 0);
      onChange(v);
    };
  };
  slider(cx - 200, "gj_songIcon_001.png", audio_->getUserMusicVolume(),
         [this](double v) { audio_->setUserMusicVolume(v); });
  slider(cx + 200, "GJ_sfxIcon_001.png", sfxVolume_, [this](double v) {
    sfxVolume_ = v;
    app->registry["userSfxVol"] = v;
  });
}

// ------------------------------------------------------------------ info popup
void GameScene::buildInfoPopup() {
  if (infoPopup_) return;
  double cx = screenWidth / 2, cy = 320, panelH = 336;
  infoPopup_ = addContainer(0, 0);
  infoPopup_->setDepth(200);
  Rectangle* rect = addRectangle(cx, cy, screenWidth, screenHeight, 0, 100.0 / 255);
  rect->setInteractive();
  infoPopup_->add(rect);
  double corner = 0.325 * app->textures->get("GJ_square02")->w;
  infoPopup_->add(drawScale9(cx, cy, 480, panelH, "GJ_square02", corner, 16777215, 1));
  Image* close = addImage(cx - 240 + 20, 172, "GJ_WebSheet", "GJ_closeBtn_001.png");
  close->setScale(0.8);
  close->setInteractive();
  infoPopup_->add(close);
  expandHitArea(close, 2);
  makeBouncyButton(close, 0.8, [this]() { closeInfoPopup(); });
  double y = 206;
  infoPopup_->add(addBitmapText(cx, y, "bigFont", "Credits", 40)->setOrigin(0.5, 0.5));
  y += 70;
  infoPopup_->add(addBitmapText(cx, y, "goldFont", "Made by RobTop Games", 40)->setOrigin(0.5, 0.5)->setScale(0.6));
  y += 60;
  infoPopup_->add(addBitmapText(cx, y, "goldFont", "Song: Stereo Madness", 40)->setOrigin(0.5, 0.5)->setScale(0.6));
  y += 30;
  BitmapText* by = addBitmapText(cx - 20, y, "goldFont", "by ForeverBound", 40);
  by->setOrigin(0.5, 0.5)->setScale(0.6);
  infoPopup_->add(by);
  double ix = cx - 10 + (0.6 * by->width()) / 2;
  Image* yt = addImage(ix + 20 + 50 - 10, y + 2, "GJ_WebSheet", "gj_ytIcon_001.png");
  yt->setScale(0.5);
  yt->setInteractive();
  infoPopup_->add(yt);
  expandHitArea(yt, 2);
  makeBouncyButton(yt, 0.5, []() { SDL_OpenURL("https://www.youtube.com/watch?v=JhKyKEDxo8Q"); });
  infoPopup_->add(addText(cx, 446, "\xC2\xA9 2026 RobTop Games. All rights reserved.", 12, 0x000000)
                      ->setOrigin(0.5, 0.5)->setAlpha(0.7));
  infoPopup_->add(addText(cx, 463, "Unauthorized copying, distribution, or hosting of this demo is prohibited.", 12, 0x000000)
                      ->setOrigin(0.5, 0.5)->setAlpha(0.7));
}

void GameScene::closeInfoPopup() {
  if (infoPopup_) { infoPopup_->destroy(); infoPopup_ = nullptr; }
}

void GameScene::expandHitArea(GameObject* g, double f) {
  double w = g->width(), h = g->height();
  double ex = (w * (f - 1)) / 2, ey = (h * (f - 1)) / 2;
  g->input->hx = -ex; g->input->hy = -ey;
  g->input->hw = w + 2 * ex; g->input->hh = h + 2 * ey;
}

Tween* GameScene::tweenScaleTo(GameObject* o, double to, double dur, EaseFn e,
                               std::function<void()> done) {
  auto from = std::make_shared<double>(1);
  return tween(o, dur, std::move(e),
               [o, from, to](double p) { o->setScale(*from + (to - *from) * p); },
               std::move(done), 0, false, 0, [o, from]() { *from = o->scaleX; });
}

void GameScene::makeBouncyButton(GameObject* g, double base, std::function<void()> onClick,
                                 std::function<bool()> enabled) {
  double big = 1.26 * base;
  Interactive* in = g->input.get();
  in->onDown = [this, g, big, enabled]() {
    if (enabled && !enabled()) return;
    g->pressed = true;
    tweens.killTweensOf(g);
    tweenScaleTo(g, big, 300, ease::bounceOut);
  };
  in->onOut = [this, g, base]() {
    if (!g->pressed) return;
    g->pressed = false;
    tweens.killTweensOf(g);
    tweenScaleTo(g, base, 400, ease::bounceOut);
  };
  in->onUp = [this, g, base, onClick]() {
    if (!g->pressed) return;
    g->pressed = false;
    tweens.killTweensOf(g);
    g->setScale(base);
    onClick();
  };
}

void GameScene::toggleFullscreen() { app->toggleFullscreen(); }

Container* GameScene::drawScale9(double x, double y, double w, double h, const std::string& key,
                                 double c, uint32_t tint, double alpha) {
  Container* cont = addContainer(x, y);
  Texture* t = app->textures->get(key);
  double tw = t->w, th = t->h;
  double iw = w - 2 * c, ih = h - 2 * c;
  struct P { double sx, sy, sw, sh, dx, dy, dw, dh; };
  P p[9] = {
      {0, 0, c, c, -w / 2, -h / 2, c, c},
      {c, 0, tw - 2 * c, c, -w / 2 + c, -h / 2, iw, c},
      {tw - c, 0, c, c, w / 2 - c, -h / 2, c, c},
      {0, c, c, th - 2 * c, -w / 2, -h / 2 + c, c, ih},
      {c, c, tw - 2 * c, th - 2 * c, -w / 2 + c, -h / 2 + c, iw, ih},
      {tw - c, c, c, th - 2 * c, w / 2 - c, -h / 2 + c, c, ih},
      {0, th - c, c, c, -w / 2, h / 2 - c, c, c},
      {c, th - c, tw - 2 * c, c, -w / 2 + c, h / 2 - c, iw, c},
      {tw - c, th - c, c, c, w / 2 - c, h / 2 - c, c, c}};
  for (int i = 0; i < 9; i++) {
    std::string name = "_s9_" + std::to_string(i);
    if (!t->has(name)) t->add(name, (int)p[i].sx, (int)p[i].sy, (int)p[i].sw, (int)p[i].sh);
    Image* im = addImage(p[i].dx, p[i].dy, key, name);
    im->setOrigin(0, 0);
    im->setDisplaySize(p[i].dw, p[i].dh);
    im->setTint(tint);
    im->setAlpha(alpha);
    cont->add(im);
  }
  return cont;
}

// ------------------------------------------------------------------ menu -> game
void GameScene::startGame() {
  if (!menuActive_) return;
  menuActive_ = false;
  slideIn_ = true;
  if (menuGlitter_) { menuGlitter_->destroy(); menuGlitter_ = nullptr; }
  if (playBtn_) {
    tweens.killTweensOf(playBtn_);
    tweenScaleTo(playBtn_, 0.01, 200, ease::quadIn, [this]() { playBtn_->destroy(); playBtn_ = nullptr; });
  }
  auto slideOut = [this](Image*& ref, double target) {
    if (!ref) return;
    Image* im = ref;
    Image** slot = &ref;
    tweenProp(im, &im->y, target, 300, ease::quadIn, [im, slot]() { im->destroy(); *slot = nullptr; });
  };
  if (robLogo_) slideOut(robLogo_, screenHeight + robLogo_->height());
  if (copyrightText_) {
    TextObj* t = copyrightText_;
    tweenProp(t, &t->y, 680, 300, ease::quadIn, [this, t]() { t->destroy(); copyrightText_ = nullptr; });
  }
  if (menuFsBtn_) slideOut(menuFsBtn_, -menuFsBtn_->height());
  if (menuInfoBtn_) slideOut(menuInfoBtn_, -menuInfoBtn_->height());
  closeInfoPopup();
  if (tryMeImg_) slideOut(tryMeImg_, -tryMeImg_->height());
  if (!downloadBtns_.empty()) {
    for (Image* item : downloadBtns_) {
      tweens.killTweensOf(item);
      tweenProp(item, &item->y, screenHeight + item->height(), 300, ease::quadIn,
                [item]() { item->destroy(); });
    }
    downloadBtns_.clear();
  }
  if (logo_) slideOut(logo_, -logo_->height());
  cameraX = -groundYOffset;
  cameraY = 0;
  cameraXRef_.value = cameraX;
  prevCameraX_ = cameraX;
  double shift = cameraX - menuCameraX_;
  level_->shiftGroundTiles(shift);
  playerWorldX = cameraX;
  state_->y = 30;
  state_->onGround = true;
  level_->additiveContainer->setVisible(true);
  level_->container->setVisible(true);
  level_->topContainer->setVisible(true);
  player_->setCubeVisible(true);
  player_->reset();
  attemptsLabel_->setVisible(attempts_ > 1);
  positionAttemptsLabel();
}

void GameScene::pushButton() {
  if (menuActive_) {
    audio_->playEffect("playSound_01", 1);
    startGame();
    return;
  }
  if (slideIn_ || state_->isDead) return;
  state_->upKeyDown = true;
  state_->upKeyPressed = true;
  if (!state_->isFlying && state_->canJump) {
    player_->updateJump(0);
    totalJumps_++;
  }
}

void GameScene::releaseButton() {
  state_->upKeyDown = false;
  state_->upKeyPressed = false;
}

void GameScene::positionMenuItems() {
  double cx = screenWidth / 2;
  if (logo_) logo_->x = cx;
  if (menuInfoBtn_) menuInfoBtn_->x = screenWidth - 30 - 3;
  if (copyrightText_) copyrightText_->x = screenWidth - 20;
  if (tryMeImg_) tryMeImg_->x = cx + 175;
  if (menuGlitter_) { menuGlitter_->x = cx; menuGlitter_->y = 320; }
  if (playBtn_) {
    playBtn_->x = cx;
    tweens.killTweensOf(playBtn_);
    playBtn_->y = 320;
    tweenProp(playBtn_, &playBtn_->y, 324, 750, ease::quadInOut, nullptr, 0, true, -1);
  }
  if (!downloadBtns_.empty()) {
    double x0 = screenWidth - 130, y0 = 555, step = 210;
    for (size_t i = 0; i < downloadBtns_.size(); i++)
      downloadBtns_[i]->setPosition(x0 - i * step, y0);
  }
}

void GameScene::positionAttemptsLabel() {
  double x = cameraX + screenWidth / 2;
  if (attempts_ > 1) x += 100;
  attemptsLabel_->setPosition(x, 150);
}

void GameScene::resetGameplayState() {
  cameraX = -groundYOffset;
  cameraY = 0;
  cameraXRef_.value = -groundYOffset;
  prevCameraX_ = -groundYOffset;
  playerWorldX = 0;
  deltaBuffer_ = 0;
  deathTimer_ = 0;
  deathSoundPlayed_ = false;
  newBestShown_ = false;
  hadNewBest_ = false;
  levelWon_ = false;
  endCameraOverride_ = false;
  endCamTween_.reset();
  spaceWasDown_ = false;
}

void GameScene::restartLevel() {
  attempts_++;
  double oldCam = cameraX;
  resetGameplayState();
  state_->reset();
  player_->reset();
  glitterEmitter_->stop();
  level_->resetObjects();
  level_->shiftGroundTiles(cameraX - oldCam);
  level_->resetGroundState();
  level_->resetColorTriggers();
  level_->resetEnterEffectTriggers();
  level_->resetVisibility();
  colorManager_->reset();
  audio_->reset();
  audio_->startMusic();
  paused_ = false;
  if (pauseContainer_) { pauseContainer_->destroy(); pauseContainer_ = nullptr; }
  pauseBtn_->setVisible(true)->setAlpha(75.0 / 255);
  attemptsLabel_->setText("Attempt " + std::to_string(attempts_));
  attemptsLabel_->setVisible(true);
  positionAttemptsLabel();
}

void GameScene::onResize() { applyScreenResize(); }

void GameScene::applyScreenResize() {
  bg_->w = screenWidth; bg_->h = screenHeight;
  pauseBtn_->x = screenWidth - 30;
  if (menuActive_) positionMenuItems();
  if (paused_ && pauseContainer_) {
    pauseContainer_->destroy();
    pauseContainer_ = nullptr;
    buildPauseOverlay();
  }
  level_->resizeScreen();
  if (!menuActive_) {
    double old = cameraX;
    cameraX = playerWorldX - groundYOffset;
    cameraXRef_.value = cameraX;
    for (Container* c : {level_->additiveContainer, level_->container, level_->topContainer}) {
      c->x = -cameraX; c->y = cameraY;
    }
    level_->shiftGroundTiles(cameraX - old);
    level_->updateGroundTiles(cameraY);
    level_->updateVisibility(cameraX);
    level_->applyEnterEffects(cameraX);
    player_->syncSprites(cameraX, cameraY, 0, playerWorldX - cameraX);
  }
}

void GameScene::updateBackground() {
  bg_->tilePositionX += (cameraX - prevCameraX_) * bgSpeedX_;
  prevCameraX_ = cameraX;
  bg_->tilePositionY = bgInitY_ - cameraY * bgSpeedY_;
}

void GameScene::updateCameraY(double dt) {
  double cy = cameraY, target = cy;
  if (level_->flyCameraTarget) target = *level_->flyCameraTarget;
  else {
    double py = state_->y, up = 140, down = 80;
    double mid = cy - unusedConst180 + 320;
    if (py > mid + up) target = py - 320 - up + unusedConst180;
    else if (py < mid - down) target = py - 320 + down + unusedConst180;
  }
  if (target < 0) target = 0;
  if (dt != 0) {
    cy += (target - cy) / (10 / dt);
    if (cy < 0) cy = 0;
    cameraY = cy;
  }
}

double GameScene::quantizeDelta(double deltaMs) {
  double v = deltaMs / 1000.0 + deltaBuffer_;
  double n = jsRound(v / fixedTimeStep);
  if (n < 0) n = 0;
  if (n > 60) n = 60;
  double q = n * fixedTimeStep;
  deltaBuffer_ = v - q;
  return 60 * q;
}

// ------------------------------------------------------------------ update
void GameScene::update(double /*time*/, double other) {
  fpsAccum_ += other;
  fpsFrames_++;
  if (fpsAccum_ >= 250) {
    fpsText_->setText(std::to_string((long long)jsRound((1000.0 * fpsFrames_) / fpsAccum_)));
    fpsAccum_ = 0;
    fpsFrames_ = 0;
  }
  if (paused_) { deltaBuffer_ = 0; return; }
  auto syncContainers = [this]() {
    for (Container* c : {level_->additiveContainer, level_->container, level_->topContainer}) {
      c->x = -cameraX; c->y = cameraY;
    }
  };
  bool spaceDown = keyDown(SDL_SCANCODE_SPACE) || keyDown(SDL_SCANCODE_UP);
  if (menuActive_) {
    if (spaceDown && !spaceWasDown_) {
      spaceWasDown_ = true;
      audio_->playEffect("playSound_01", 1);
      startGame();
      return;
    }
    spaceWasDown_ = spaceDown;
    double f = std::min((other / 1000.0) * 60, 2.0), k = 0.25;
    menuCameraX_ = menuCameraX_ + f * gravityConst * physicsConst09 * k;
    double saved = cameraX;
    cameraX = menuCameraX_;
    updateBackground();
    cameraX = saved;
    prevCameraX_ = menuCameraX_;
    cameraXRef_.value = menuCameraX_;
    level_->stepGroundAnimation(other / 1000.0);
    level_->updateGroundTiles(cameraY);
    return;
  }
  if (slideIn_) {
    double q = quantizeDelta(other);
    playerWorldX += q * gravityConst * physicsConst09;
    double k = 0.25;
    double base = slideGroundX_ ? *slideGroundX_ : cameraX;  // JS `||`
    if (slideGroundX_ && *slideGroundX_ == 0) base = cameraX;
    slideGroundX_ = base + q * gravityConst * physicsConst09 * k;
    cameraXRef_.value = *slideGroundX_;
    double sx = playerWorldX - cameraX;
    player_->updateGroundRotation(q * physicsConst09);
    player_->syncSprites(cameraX, cameraY, other / 1000.0, sx);
    syncContainers();
    level_->updateVisibility(cameraX);
    updateBackground();
    level_->stepGroundAnimation(other / 1000.0);
    level_->updateGroundTiles(cameraY);
    if (playerWorldX >= 0) {
      slideIn_ = false;
      deltaBuffer_ = 0;
      playerWorldX = 0;
      cameraX = playerWorldX - groundYOffset;
      cameraXRef_.value = cameraX;
      double shift = cameraX - *slideGroundX_;
      level_->shiftGroundTiles(shift);
      if (firstPlay_) { firstPlay_ = false; audio_->startMusic(); }
      pauseBtn_->setVisible(true)->setAlpha(0);
      tweenProp(pauseBtn_, &pauseBtn_->alpha, 75.0 / 255, 500, ease::linear);
    }
    return;
  }
  if (spaceDown && !spaceWasDown_) pushButton();
  else if (!spaceDown && spaceWasDown_) releaseButton();
  spaceWasDown_ = spaceDown;
  if (pointer.isDown && !state_->upKeyDown && !state_->isDead) state_->upKeyDown = true;
  level_->updateEndPortalY(cameraY, state_->isFlying);
  if (!levelWon_ && !state_->isDead && level_->endXPos > 0) {
    if (playerWorldX >= level_->endXPos - 600) {
      levelWon_ = true;
      endPortalGameY_ = level_->endPortalGameY_ ? level_->endPortalGameY_ : 240;
      triggerEndPortal();
    }
  }
  if (levelWon_) {
    deltaBuffer_ = 0;
    if (endCamTween_) {
      cameraX = endCamTween_->fromX + (endCamTween_->toX - endCamTween_->fromX) * endCamTween_->p;
      cameraY = endCamTween_->fromY + (endCamTween_->toY - endCamTween_->fromY) * endCamTween_->p;
    }
    cameraXRef_.value = cameraX;
    syncContainers();
    updateBackground();
    level_->stepGroundAnimation(other / 1000.0);
    level_->updateGroundTiles(cameraY);
    return;
  }
  if (state_->isDead) {
    if (!deathSoundPlayed_) {
      audio_->stopMusic();
      audio_->playEffect("explode_11", 0.65);
      deathSoundPlayed_ = true;
    }
    if (!newBestShown_) {
      newBestShown_ = true;
      double end = level_->endXPos ? level_->endXPos : 6000;
      lastPercent_ = std::min(99.0, std::max(0.0, std::floor((playerWorldX / end) * 100)));
      if (lastPercent_ > bestPercent_) {
        bestPercent_ = lastPercent_;
        hadNewBest_ = true;
        showNewBest();
      }
    }
    player_->updateExplosionPieces(other);
    deathTimer_ += other;
    double limit = hadNewBest_ ? 1400 : 1000;
    if (deathTimer_ > limit) restartLevel();
    return;
  }
  playTime_ += other / 1000.0;
  audio_->update(other / 1000.0);
  level_->updateAudioScale(audio_->getMeteringValue());
  double q = quantizeDelta(other);
  double steps = q > 0 ? std::max(1.0, jsRound(4 * q)) : 0;
  if (steps > 60) steps = 60;
  double sub = steps > 0 ? q / steps : 0;
  double step = sub * physicsConst09;
  double yBefore = state_->y;
  for (int i = 0; i < (int)steps; i++) {
    state_->lastY = state_->y;
    player_->updateJump(step);
    state_->y += state_->yVelocity * step;
    player_->checkCollisions(playerWorldX - groundYOffset);
    playerWorldX += sub * gravityConst * physicsConst09;
    if (!state_->isFlying) {
      if (state_->onGround) player_->updateGroundRotation(step);
      else if (player_->rotateActionActive) player_->updateRotateAction(fixedTimeStep);
    }
  }
  state_->lastY = yBefore;
  if (!endCameraOverride_) {
    double cx = playerWorldX - groundYOffset;
    if (level_->endXPos > 0) {
      double endCam = level_->endXPos - screenWidth;
      if (cx >= endCam - 200) {
        endCameraOverride_ = true;
        cameraX = cx;
        double toY = -140 + (level_->endPortalGameY_ ? level_->endPortalGameY_ : 240);
        auto tw = std::make_shared<EndCam>();
        tw->p = 0; tw->fromX = cameraX; tw->toX = endCam; tw->fromY = cameraY; tw->toY = toY;
        endCamTween_ = tw;
        const double n = 1.8;
        tween(tw.get(), 1200,
              [n](double v) {
                return v < 0.5 ? std::pow(2 * v, n) / 2 : 1 - std::pow(2 * (1 - v), n) / 2;
              },
              [tw](double p) { tw->p = p; });
      } else cameraX = cx;
    } else cameraX = cx;
  }
  if (endCameraOverride_ && endCamTween_) {
    cameraX = endCamTween_->fromX + (endCamTween_->toX - endCamTween_->fromX) * endCamTween_->p;
    cameraY = endCamTween_->fromY + (endCamTween_->toY - endCamTween_->fromY) * endCamTween_->p;
  }
  cameraXRef_.value = cameraX;
  if (!endCameraOverride_) updateCameraY(q);
  syncContainers();
  double pwx = playerWorldX;
  for (const ColorTrigger& t : level_->checkColorTriggers(pwx)) {
    colorManager_->triggerColor(t.index, t.color, t.duration);
    if (t.tintGround) colorManager_->triggerColor(num31, t.color, t.duration);
  }
  colorManager_->step(other / 1000.0);
  bg_->setTint(colorManager_->getHex(num30));
  level_->setGroundColor(colorManager_->getHex(num31));
  level_->updateVisibility(cameraX);
  level_->checkEnterEffectTriggers(pwx);
  level_->applyEnterEffects(cameraX);
  glitterCenterX_ = cameraX + screenWidth / 2;
  glitterCenterY_ = yFlipBase - cameraY;
  updateBackground();
  level_->stepGroundAnimation(other / 1000.0);
  level_->updateGroundTiles(cameraY);
  if (state_->isFlying) player_->updateShipRotation(q);
  double sx = playerWorldX - cameraX;
  player_->syncSprites(cameraX, cameraY, other / 1000.0, sx);
}

void GameScene::showNewBest() {
  double cx = screenWidth / 2;
  Image* img = addImage(0, 0, "GJ_WebSheet", "GJ_newBest_001.png");
  img->setOrigin(0.5, 1);
  BitmapText* txt = addBitmapText(0, 2, "bigFont", std::to_string((int)lastPercent_) + "%", 65);
  txt->setOrigin(0.5, 0)->setScale(1.1);
  Container* c = addContainer(cx, 300);
  c->add(img);
  c->add(txt);
  c->setDepth(60)->setScale(0.01);
  auto from = std::make_shared<double>(0.01);
  tween(c, 400, [](double v) { return ease::elasticOut(v, 1, 0.6); },
        [c, from](double p) { c->setScale(*from + (1 - *from) * p); },
        [this, c]() {
          auto from2 = std::make_shared<double>(1);
          tween(c, 200, ease::quadIn,
                [c, from2](double p) { c->setScale(*from2 + (0.01 - *from2) * p); },
                [c]() { c->setVisible(false); c->destroy(); }, 700, false, 0,
                [c, from2]() { *from2 = c->scaleX; });
        },
        0, false, 0, [c, from]() { *from = c->scaleX; });
}

void GameScene::triggerEndPortal() {
  player_->playEndAnimation(level_->endXPos, [this]() { levelComplete(); }, endPortalGameY_);
}

void GameScene::levelComplete() {
  double x = level_->endXPos - cameraX;
  double y = flipY(endPortalGameY_) + cameraY;
  for (int i = 0; i < 5; i++)
    clock.delayedCall(50 * i, [this, x, y]() {
      drawExpandingRing(this, x, y, 10, screenWidth, 500, false, true, colorGreenTint);
    });
  drawExpandingRing(this, x, y, 10, 1000, 500, true, false, colorGreenTint);
  showCompleteEffect();
}

// ------------------------------------------------------------------ level complete
namespace {
// IIFE inside _showCompleteEffect: the fan of light rays behind the end portal.
void spawnRays(GameScene* scene, double x, double y, uint32_t color) {
  const int rays = 8;
  const double baseW = 2, maxW = 60, maxWJit = 40;
  double height = jsRound(std::sqrt(screenWidth * screenWidth + 102400)) + 65;
  const double dur0 = 180, durJ = 40, stagger = 195, startDelay = 40, delayJ = 40;
  const double alpha0 = 155.0 / 255, alphaJ = 100.0 / 255, fadeWait = 400;
  const double angleBase = -135, angleStep = 90.0 / rays;
  std::vector<double> angles;
  for (int i = 0; i < rays; i++) angles.push_back(angleBase + i * angleStep);
  for (int i = (int)angles.size() - 1; i > 0; i--) {
    int j = (int)std::floor(random01() * (i + 1));
    std::swap(angles[i], angles[j]);
  }
  double maxDelay = 0;
  auto list = std::make_shared<std::vector<Graphics*>>();
  for (int i = 0; i < rays; i++) {
    double delay = i * stagger + startDelay + delayJ * (2 * random01() - 1);
    double tw = maxW + maxWJit * (2 * random01() - 1);
    double dur = dur0 + durJ * (2 * random01() - 1);
    double a = std::min(1.0, std::max(0.0, alpha0 + alphaJ * (2 * random01() - 1)));
    double ang = angles[i] + angleStep * random01() + 180;
    Graphics* g = scene->addGraphics();
    g->setDepth(-1)->setBlendMode(Blend::Add)->setPosition(x, y)->setAngle(ang)->setAlpha(a)->setVisible(false);
    auto st = std::make_shared<std::pair<double, double>>(1.0, baseW);  // h, w
    scene->clock.delayedCall(std::max(0.0, delay), [=]() {
      g->setVisible(true);
      auto from = std::make_shared<std::pair<double, double>>(0, 0);
      scene->tween(g, dur, ease::quadOut,
                   [=](double p) {
                     st->first = from->first + (height - from->first) * p;
                     st->second = from->second + (tw - from->second) * p;
                     double top = baseW + (st->second - baseW) / 4;
                     g->clear();
                     g->fillStyle(color, 1);
                     g->beginPath();
                     g->moveTo(-top / 2, 0);
                     g->lineTo(top / 2, 0);
                     g->lineTo(st->second / 2, st->first);
                     g->lineTo(-st->second / 2, st->first);
                     g->closePath();
                     g->fillPath();
                   },
                   nullptr, 0, false, 0, [=]() { *from = *st; });
    });
    if (delay > maxDelay) maxDelay = delay;
    list->push_back(g);
  }
  scene->clock.delayedCall(maxDelay + fadeWait, [=]() {
    for (Graphics* g : *list) {
      double d = 200 * random01(), dur = 400 + 100 * (2 * random01() - 1);
      scene->tweenProp(g, &g->alpha, 0, dur, ease::linear, [g]() { g->destroy(); }, d);
    }
  });
}
}  // namespace

void GameScene::showCompleteEffect() {
  audio_->fadeOutMusic(1500);
  app->sound.play("endStart_02", 0.8);
  spawnRays(this, level_->endXPos - cameraX + 60, flipY(endPortalGameY_) + cameraY, colorGreenTint);
  cameraShake(1950, 0.004);
  clock.delayedCall(1950, [this]() { showCompleteText(); });
}

void GameScene::showCompleteText() {
  double cx = screenWidth / 2;
  Image* img = addImage(cx, 250, "GJ_WebSheet", "GJ_levelComplete_001.png");
  img->setDepth(60)->setScale(0.01);
  auto from = std::make_shared<double>(0.01);
  tween(img, 660, [](double v) { return ease::elasticOut(v, 1, 0.6); },
        [img, from](double p) { img->setScale(*from + (1.1 - *from) * p); },
        [this, img]() {
          auto from2 = std::make_shared<double>(1.1);
          tween(img, 220, ease::quadIn,
                [img, from2](double p) { img->setScale(*from2 + (0.01 - *from2) * p); },
                [img]() { img->setVisible(false); img->destroy(); }, 880, false, 0,
                [img, from2]() { *from2 = img->scaleX; });
        },
        0, false, 0, [img, from]() { *from = img->scaleX; });
  uint32_t tints[2] = {colorGreenTint, 16777215};
  for (int i = 0; i < 2; i++) {
    EmitterConfig c;
    c.speed = Op::R(300, 700); c.angle = Op::R(0, 360);
    c.scaleStart = 0.4; c.scaleEnd = 0.13; c.lifespan = Op::R(0, 1000);
    c.quantity = 50; c.stopAfter = 200; c.blend = Blend::Add;
    c.tintStart = c.tintEnd = tints[i];
    c.x = Op::R(-800, 800); c.y = Op::R(-80, 80);
    addParticles(cx, 250, "GJ_WebSheet", c)->setDepth(59);
  }
  double ex = level_->endXPos - cameraX, ey = flipY(endPortalGameY_) + cameraY;
  drawExpandingRing(this, ex, ey, 10, screenWidth, 800, true, false, colorGreenTint);
  drawExpandingRing(this, cx, 250, 10, 1000, 800, true, false, colorGreenTint);
  for (int i = 0; i < 5; i++)
    clock.delayedCall(50 * i, [this, ex, ey]() {
      drawExpandingRing(this, ex, ey, 10, screenWidth, 500, false, true, colorGreenTint);
    });
  for (int i = 0; i < 10; i++) {
    double d = 150 * i + (160 * random01() - 80);
    clock.delayedCall(std::max(0.0, d), [this]() { spawnFinishParticles(this, colorGreenTint, colorCyanTint); });
  }
  clock.delayedCall(1500, [this]() { showEndLayer(); });
}

void GameScene::showEndLayer() {
  if (pauseBtn_) tweenProp(pauseBtn_, &pauseBtn_->alpha, 0, 300, ease::linear);
  double cx = screenWidth / 2, cy = 320;
  endLayerOverlay_ = addRectangle(cx, cy, screenWidth, screenHeight, 0, 0);
  endLayerOverlay_->setDepth(200);
  endLayerOverlay_->setInteractive();
  endLayerInternal_ = addContainer(0, -640);
  endLayerInternal_->setDepth(201);
  tweenProp(endLayerOverlay_, &endLayerOverlay_->alpha, 100.0 / 255, 1000, ease::linear);
  auto p = std::make_shared<double>(0);
  Container* internal = endLayerInternal_;
  tween(p.get(), 1000, ease::bounceOut,
        [this, p, internal](double v) { *p = v; internal->y = 650 * v - 640; },
        [this]() { playStarAward(); });
  const double panelW = 712, panelH = 460;
  double left = (screenWidth - panelW) / 2;
  internal->add(addRectangle(left + 356, 310, panelW, panelH, 0, 180.0 / 255));
  Frame* side = frameOf(this, "GJ_table_side_001.png");
  double sideScale = side ? panelH / side->height : 1;
  internal->add(addImage(left - 40, 80, "GJ_WebSheet", "GJ_table_side_001.png")->setOrigin(0, 0)->setScale(1, sideScale));
  {
    Image* r = addImage(left + panelW + 40, 80, "GJ_WebSheet", "GJ_table_side_001.png");
    r->setOrigin(1, 0);
    r->setFlipX(true);
    r->setScale(1, sideScale);
    internal->add(r);
  }
  Image* top = addImage(left + 356, 70, "GJ_WebSheet", "GJ_table_top_001.png");
  internal->add(top);
  internal->add(addImage(left + 356, 560, "GJ_WebSheet", "GJ_table_bottom_001.png"));
  double chainY = top->y - 65;
  internal->add(addImage(cx - 312, chainY, "GJ_WebSheet", "chain_01_001.png")->setOrigin(0.5, 1));
  internal->add(addImage(cx + 312, chainY, "GJ_WebSheet", "chain_01_001.png")->setOrigin(0.5, 1));
  internal->add(addImage(cx, 170, "GJ_WebSheet", "GJ_levelComplete_001.png")->setScale(0.8));
  const double ts = 0.8;
  double ty = 250;
  internal->add(addBitmapText(cx, ty, "goldFont", "Attempts: " + std::to_string(attempts_), 40)->setOrigin(0.5, 0.5)->setScale(ts));
  ty += 48;
  internal->add(addBitmapText(cx, ty, "goldFont", "Jumps: " + std::to_string(totalJumps_), 40)->setOrigin(0.5, 0.5)->setScale(ts));
  ty += 48;
  long long secs = (long long)std::floor(playTime_);
  long long hh = secs / 3600, mm = (secs % 3600) / 60, ss = secs % 60;
  auto two = [](long long v) { char b[32]; std::snprintf(b, sizeof b, "%02lld", v); return std::string(b); };
  std::string timeStr = hh > 0 ? two(hh) + ":" + two(mm) + ":" + two(ss) : two(mm) + ":" + two(ss);
  double timeY = ty;
  internal->add(addBitmapText(cx, ty, "goldFont", "Time: " + timeStr, 40)->setOrigin(0.5, 0.5)->setScale(ts));
  static const char* phrases[] = {"Awesome!", "Good\nJob!", "Well\nDone!", "Impressive!", "Amazing!",
                                  "Incredible!", "Skillful!", "Brilliant!", "Not\nbad!", "Warp\nSpeed!",
                                  "Challenge\nBreaker!", "Reflex\nMaster!", "I am\nspeechless...",
                                  "You are...\nThe One!", "How is this\npossible!?", "You beat\nme..."};
  const char* phrase = phrases[(int)std::floor(random01() * 16)];
  const double off = 225;
  {
    BitmapText* pt = addBitmapText(cx + off, timeY, "bigFont", phrase, 40);
    pt->setOrigin(0.5, 0.5)->setScale(0.8);
    pt->setCenterAlign();
    internal->add(pt);
  }
  internal->add(addImage(cx - off, 352.5, "GJ_WebSheet", "getIt_001.png")->setScale(1 / 1.5));
  struct DL { const char* key; const char* url; };
  const DL stores[] = {
      {"downloadApple_001", "https://apps.apple.com/us/app/geometry-dash/id625334537"},
      {"downloadGoogle_001", "https://play.google.com/store/apps/details?id=com.robtopx.geometryjump&hl=en"},
      {"downloadSteam_001", "https://store.steampowered.com/app/322170/Geometry_Dash"}};
  for (int i = 0; i < 3; i++) {
    double s = 1 / 1.5;
    Image* im = addImage(cx + (i - 1) * off, 437.5, "GJ_WebSheet", std::string(stores[i].key) + ".png");
    im->setScale(s);
    im->setInteractive();
    internal->add(im);
    std::string url = stores[i].url;
    makeBouncyButton(im, s, [url]() { SDL_OpenURL(url.c_str()); });
  }
  endStarX_ = cx + off;
  endStarY_ = timeY - 77.5;
  struct B { const char* frame; double dx; std::function<void()> action; };
  std::vector<B> bs = {
      {"GJ_replayBtn_001.png", -200, [this]() { hideEndLayer([this]() { restartLevel(); }); }},
      {"GJ_menuBtn_001.png", 200, [this]() {
         audio_->playEffect("quitSound_01");
         audio_->stopMusic();
         app->registry["fadeInFromBlack"] = 1;
         cameraFadeOut(400, [this](double prog) { if (prog >= 1) app->requestSceneRestart(); });
       }}};
  for (B& b : bs) {
    Image* im = addImage(cx + b.dx, 555, "GJ_WebSheet", b.frame);
    im->setInteractive();
    internal->add(im);
    makeBouncyButton(im, 1, b.action);
  }
}

void GameScene::playStarAward() {
  if (!endLayerInternal_) return;
  double sx = endStarX_, sy = endStarY_;
  Image* star = addImage(sx, sy, "GJ_WebSheet", "GJ_bigStar_001.png");
  star->setScale(3)->setAlpha(0);
  endLayerInternal_->add(star);
  tween(star, 300, ease::bounceOut, [star](double p) { star->setScale(3 + (0.8 - 3) * p); star->alpha = p; });
  clock.delayedCall(100, [this, sx, sy]() {
    audio_->playEffect("highscoreGet02");
    double px_ = sx, py = sy + (endLayerInternal_ ? endLayerInternal_->y : 0);
    EmitterConfig c;
    c.speed = Op::R(200, 600); c.angle = Op::R(0, 360);
    c.scaleStart = 0.5; c.scaleEnd = 0; c.alphaStart = 1; c.alphaEnd = 0;
    c.lifespan = Op::R(200, 600); c.quantity = 30; c.stopAfter = 30; c.blend = Blend::Add;
    c.tintStart = c.tintEnd = 16776960;
    addParticles(px_, py, "GJ_WebSheet", c)->setDepth(202);
    Graphics* g = addGraphics();
    g->setDepth(202)->setBlendMode(Blend::Add);
    tween(g, 400, ease::quadOut,
          [g, px_, py](double t) {
            g->clear();
            g->fillStyle(16776960, 1 - t);
            g->fillCircle(px_, py, 20 + 200 * t);
          },
          [g]() { g->destroy(); });
  });
}

void GameScene::hideEndLayer(std::function<void()> cb) {
  if (!endLayerInternal_) { if (cb) cb(); return; }
  auto p = std::make_shared<double>(0);
  tween(p.get(), 500,
        [](double v) { return v < 0.5 ? std::pow(2 * v, 2) / 2 : 1 - std::pow(2 * (1 - v), 2) / 2; },
        [this, p](double v) { *p = v; if (endLayerInternal_) endLayerInternal_->y = -640 * v; },
        [this, cb]() {
          endLayerInternal_->destroy();
          endLayerInternal_ = nullptr;
          if (endLayerOverlay_) { endLayerOverlay_->destroy(); endLayerOverlay_ = nullptr; }
          if (cb) cb();
        });
  if (endLayerOverlay_) tweenProp(endLayerOverlay_, &endLayerOverlay_->alpha, 0, 500, ease::linear);
}
