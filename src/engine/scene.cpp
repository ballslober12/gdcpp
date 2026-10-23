#include "scene.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <cstdio>

namespace px {

// ============================================================ Scene
Scene::~Scene() {}

Image* Scene::addImage(double x, double y, const std::string& key, const std::string& frame) {
  Texture* t = app->textures->get(key);
  Frame* f = nullptr;
  if (t) f = frame.empty() ? t->base() : t->get(frame);
  if (t && !f) { SDL_Log("missing frame %s/%s", key.c_str(), frame.c_str()); f = t->base(); }
  Image* im = make<Image>(t, f);
  im->x = x; im->y = y;
  return im;
}
Container* Scene::addContainer(double x, double y) {
  Container* c = make<Container>();
  c->originX = c->originY = 0;
  c->x = x; c->y = y;
  return c;
}
Rectangle* Scene::addRectangle(double x, double y, double w, double h, uint32_t color, double fa) {
  Rectangle* r = make<Rectangle>(w, h, color, fa);
  r->x = x; r->y = y;
  return r;
}
TileSprite* Scene::addTileSprite(double x, double y, double w, double h, const std::string& key) {
  TileSprite* t = make<TileSprite>(app->textures->get(key), w, h);
  t->x = x; t->y = y;
  return t;
}
Graphics* Scene::addGraphics() {
  Graphics* g = make<Graphics>();
  g->originX = g->originY = 0;
  return g;
}
BitmapText* Scene::addBitmapText(double x, double y, const std::string& font,
                                 const std::string& text, double size) {
  const BitmapFont& f = app->fonts[font];
  BitmapText* b = make<BitmapText>(&f, app->textures->get(f.textureKey), text, size);
  b->x = x; b->y = y;
  return b;
}
TextObj* Scene::addText(double x, double y, const std::string& text, int px_, uint32_t color) {
  TextObj* t = make<TextObj>(text, px_, color);
  t->x = x; t->y = y;
  return t;
}
ParticleEmitter* Scene::addParticles(double x, double y, const std::string& texKey,
                                     const EmitterConfig& cfg) {
  ParticleEmitter* p = make<ParticleEmitter>(app->textures->get(texKey), cfg);
  p->originX = p->originY = 0;
  p->x = x; p->y = y;
  return p;
}

void Scene::registerInteractive(GameObject* o) { interactives_.push_back(o); }

void Scene::destroyObject(GameObject* o) {
  if (!o || o->dead) return;
  if (auto* c = dynamic_cast<Container*>(o)) {
    std::vector<GameObject*> kids = c->list;
    for (GameObject* k : kids) destroyObject(k);
    c->list.clear();
  }
  o->dead = true;
  if (o->parent) o->parent->remove(o);
  displayList.erase(std::remove(displayList.begin(), displayList.end(), o), displayList.end());
  interactives_.erase(std::remove(interactives_.begin(), interactives_.end(), o),
                      interactives_.end());
  tweens.killTweensOf(o);
  if (over_ == o) over_ = nullptr;
  if (dragging_ == o) dragging_ = nullptr;
}

void Scene::sweep() {
  all_.erase(std::remove_if(all_.begin(), all_.end(),
                            [](const std::unique_ptr<GameObject>& g) { return g->dead; }),
             all_.end());
}

bool Scene::keyDown(SDL_Scancode sc) const { return SDL_GetKeyboardState(nullptr)[sc] != 0; }

// ---- tweens
Tween* Scene::tween(const void* owner, double duration, EaseFn e, std::function<void(double)> upd,
                    std::function<void()> done, double delay, bool yoyo, int repeat,
                    std::function<void()> onStart) {
  Tween t;
  t.owner = owner; t.duration = duration; t.ease = std::move(e);
  t.onUpdate = std::move(upd); t.onComplete = std::move(done);
  t.delay = delay; t.yoyo = yoyo; t.repeat = repeat; t.onStart = std::move(onStart);
  return tweens.add(std::move(t));
}
Tween* Scene::tweenProp(GameObject* o, double* prop, double to, double duration, EaseFn e,
                        std::function<void()> done, double delay, bool yoyo, int repeat) {
  auto from = std::make_shared<double>(0);
  return tween(
      o, duration, std::move(e),
      [prop, from, to](double p) { *prop = *from + (to - *from) * p; }, std::move(done), delay,
      yoyo, repeat, [prop, from]() { *from = *prop; });
}

// ---- camera
void Scene::cameraShake(double ms, double intensity) {
  if (shakeLeft_ > 0) return;
  shakeDur_ = shakeLeft_ = ms; shakeInt_ = intensity;
}
void Scene::cameraFadeIn(double ms) {
  fadeActive_ = true; fadeOut_ = false; fadeDur_ = ms; fadeEl_ = 0; fadeAlpha_ = 1; fadeCb_ = nullptr;
}
void Scene::cameraFadeOut(double ms, std::function<void(double)> cb) {
  fadeActive_ = true; fadeOut_ = true; fadeDur_ = ms; fadeEl_ = 0; fadeAlpha_ = 0; fadeCb_ = std::move(cb);
}

// ---- frame update
void Scene::engineUpdate(double time, double delta) {
  time_ = time;
  // updateList: emitters tick before everything else
  std::vector<ParticleEmitter*> ems;
  for (auto& g : all_) if (!g->dead) if (auto* e = dynamic_cast<ParticleEmitter*>(g.get())) ems.push_back(e);
  for (auto* e : ems) if (!e->dead) e->update(delta);
  tweens.update(delta);
  clock.update(delta);
  // camera shake
  if (shakeLeft_ > 0) {
    shakeLeft_ -= delta;
    if (shakeLeft_ > 0) {
      shakeX_ = (random01() * shakeInt_ * app->screenW * 2 - shakeInt_ * app->screenW);
      shakeY_ = (random01() * shakeInt_ * app->screenH * 2 - shakeInt_ * app->screenH);
    } else shakeX_ = shakeY_ = 0;
  }
  // camera fade
  if (fadeActive_) {
    fadeEl_ += delta;
    double prog = fadeDur_ > 0 ? std::min(fadeEl_ / fadeDur_, 1.0) : 1.0;
    fadeAlpha_ = fadeOut_ ? prog : 1 - prog;
    if (fadeOut_ && fadeCb_) { auto cb = fadeCb_; cb(prog); }
    if (prog >= 1) fadeActive_ = false;
  }
  update(time, delta);
  sweep();
}

void Scene::render(Renderer& r) {
  r.setOffset(shakeX_, shakeY_);
  std::vector<GameObject*> sorted = displayList;
  std::stable_sort(sorted.begin(), sorted.end(),
                   [](GameObject* a, GameObject* b) { return a->depth < b->depth; });
  for (GameObject* o : sorted) {
    if (!o || o->dead || !o->willRender()) continue;
    o->render(r, o->local(), o->alpha);
  }
  r.setOffset(0, 0);
  if (fadeAlpha_ > 0) r.fillRect(0, 0, app->screenW, app->screenH, 0x000000, fadeAlpha_);
}

// ---- input
Mat Scene::worldOf(const GameObject* o, bool* visible) const {
  std::vector<const GameObject*> chain;
  for (const GameObject* c = o; c; c = c->parent) chain.push_back(c);
  Mat m;
  bool vis = true;
  for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
    m = m * (*it)->local();
    if (!(*it)->willRender()) vis = false;
  }
  if (visible) *visible = vis;
  return m;
}

bool Scene::orderAbove(const GameObject* a, const GameObject* b) const {
  auto path = [&](const GameObject* o) {
    std::vector<std::pair<double, double>> p;
    std::vector<const GameObject*> chain;
    for (const GameObject* c = o; c; c = c->parent) chain.push_back(c);
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
      const GameObject* c = *it;
      if (!c->parent) {
        auto f = std::find(displayList.begin(), displayList.end(), c);
        p.push_back({c->depth, (double)(f - displayList.begin())});
      } else {
        auto& l = static_cast<Container*>(c->parent)->list;
        auto f = std::find(l.begin(), l.end(), c);
        p.push_back({0, (double)(f - l.begin())});
      }
    }
    return p;
  };
  return path(a) > path(b);
}

GameObject* Scene::hitTest(double px_, double py_) {
  GameObject* best = nullptr;
  for (GameObject* o : interactives_) {
    if (o->dead || !o->input) continue;
    bool vis;
    Mat w = worldOf(o, &vis);
    if (!vis) continue;
    Mat inv;
    if (!w.invert(inv)) continue;
    double lx, ly;
    inv.apply(px_, py_, lx, ly);
    lx += o->originX * o->width();
    ly += o->originY * o->height();
    Interactive* in = o->input.get();
    if (lx < in->hx || lx > in->hx + in->hw || ly < in->hy || ly > in->hy + in->hh) continue;
    if (!best || orderAbove(o, best)) best = o;
  }
  return best;
}

void Scene::updateOver() {
  GameObject* top = hitTest(pointer.x, pointer.y);
  if (top != over_) {
    GameObject* prev = over_;
    over_ = top;
    if (prev && !prev->dead && prev->input && prev->input->onOut) prev->input->onOut();
  }
}

void Scene::handleEvent(const SDL_Event& e) {
  auto toLogical = [&](int wx, int wy) {
    float lx, ly;
    SDL_RenderWindowToLogical(app->renderer.sdl(), wx, wy, &lx, &ly);
    pointer.x = lx; pointer.y = ly;
  };
  switch (e.type) {
    case SDL_MOUSEMOTION:
      toLogical(e.motion.x, e.motion.y);
      if (dragging_ && pointer.isDown) {
        Interactive* in = dragging_->input.get();
        if (in && in->onDrag)
          in->onDrag(pointer.x - (in->ptrStartX - in->dragStartX),
                     pointer.y - (in->ptrStartY - in->dragStartY));
      } else updateOver();
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (e.button.button != SDL_BUTTON_LEFT) break;
      toLogical(e.button.x, e.button.y);
      pointer.isDown = true;
      {
        GameObject* t = hitTest(pointer.x, pointer.y);
        over_ = t;
        if (t && t->input) {
          Interactive* in = t->input.get();
          if (in->draggable) {
            dragging_ = t;
            in->dragStartX = t->x; in->dragStartY = t->y;
            in->ptrStartX = pointer.x; in->ptrStartY = pointer.y;
          }
          if (in->onDown) in->onDown();
        }
      }
      if (onPointerDown) onPointerDown();
      break;
    case SDL_MOUSEBUTTONUP:
      if (e.button.button != SDL_BUTTON_LEFT) break;
      toLogical(e.button.x, e.button.y);
      pointer.isDown = false;
      {
        GameObject* drag = dragging_;
        dragging_ = nullptr;
        GameObject* t = hitTest(pointer.x, pointer.y);
        if (drag && !drag->dead && drag->input && drag->input->onUp) drag->input->onUp();
        if (t && t != drag && t->input && t->input->onUp) t->input->onUp();
      }
      if (onPointerUp) onPointerUp();
      break;
    case SDL_KEYDOWN:
      if (!e.key.repeat && onKeyDown) onKeyDown(e.key.keysym.sym);
      break;
    default: break;
  }
}

// ============================================================ App
std::string App::assetPath(const std::string& rel) const { return base_ + rel; }

bool App::init() {
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
    SDL_Log("SDL_Init: %s", SDL_GetError());
    return false;
  }
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();
  window = SDL_CreateWindow("Geometry Dash", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            (int)screenW, (int)screenH,
                            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window) { SDL_Log("window: %s", SDL_GetError()); return false; }
  SDL_Renderer* r = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
                                                        SDL_RENDERER_TARGETTEXTURE);
  if (!r) r = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
  if (!r) { SDL_Log("renderer: %s", SDL_GetError()); return false; }
  if (!renderer.init(r)) return false;
  applyLogicalSize();
  textures = std::make_unique<TextureManager>(r);
  sound.init();

  // locate the assets folder
  std::vector<std::string> cands;
  if (char* bp = SDL_GetBasePath()) { cands.push_back(std::string(bp) + "assets/"); cands.push_back(std::string(bp) + "../assets/"); SDL_free(bp); }
  cands.push_back("assets/"); cands.push_back("../assets/");
  for (auto& c : cands) {
    if (FILE* f = std::fopen((c + "GJ_WebSheet.json").c_str(), "rb")) { std::fclose(f); base_ = c; break; }
  }
  if (base_.empty()) { SDL_Log("assets folder not found"); return false; }
  setUiFontPath(assetPath("fonts/LiberationSans-Regular.ttf"));
  for (double& h : history_) h = 1000.0 / 60.0;
  return true;
}

void App::applyLogicalSize() { SDL_RenderSetLogicalSize(renderer.sdl(), (int)screenW, (int)screenH); }

void App::toggleFullscreen() {
  fullscreen = !fullscreen;
  SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void App::present() {
  renderer.beginFrame();
  if (scene_) scene_->render(renderer);
  renderer.endFrame();
}

void App::run() {
  if (next_) { cur_ = next_; scene_ = next_(); next_ = nullptr; scene_->create(); }
  Uint64 freq = SDL_GetPerformanceFrequency();
  Uint64 start = SDL_GetPerformanceCounter(), last = start;
  bool hidden = false;
  while (running_) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) running_ = false;
      else if (e.type == SDL_WINDOWEVENT) {
        switch (e.window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
          case SDL_WINDOWEVENT_RESIZED: {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            double nw = fullscreen && h > 0 ? std::round(screenH * w / h) : 1138;
            if (nw != screenW) {
              screenW = nw;
              applyLogicalSize();
              if (onScreenWidthChanged) onScreenWidthChanged(screenW);
              if (scene_) scene_->onResize();
            }
            break;
          }
          case SDL_WINDOWEVENT_HIDDEN:
          case SDL_WINDOWEVENT_MINIMIZED:
            if (!hidden) { hidden = true; sound.suspend(true); if (scene_) scene_->onSuspend(true); }
            break;
          case SDL_WINDOWEVENT_SHOWN:
          case SDL_WINDOWEVENT_RESTORED:
            if (hidden) { hidden = false; sound.suspend(false); if (scene_) scene_->onSuspend(false); }
            break;
        }
      } else if (scene_) scene_->handleEvent(e);
    }
    Uint64 now = SDL_GetPerformanceCounter();
    double before = (double)(now - last) * 1000.0 / freq;
    last = now;
    // Phaser TimeStep delta smoothing (10 frame average, minFps 5)
    if (before < 0) before = 0;
    if (before > 200) before = history_[histIdx_];
    before = std::min(before, 200.0);
    history_[histIdx_] = before;
    histIdx_ = (histIdx_ + 1) % 10;
    double avg = 0;
    for (double h : history_) avg += h;
    double delta = avg / 10.0;
    double time = (double)(now - start) * 1000.0 / freq;

    if (scene_) {
      loopDelta = delta;
      scene_->engineUpdate(time, delta);
      present();
    }
    if (restart_ && cur_) { restart_ = false; scene_.reset(); scene_ = cur_(); scene_->create(); }
    else if (next_) { cur_ = next_; next_ = nullptr; scene_.reset(); scene_ = cur_(); scene_->create(); }
  }
}

void App::shutdown() {
  scene_.reset();
  textures.reset();
  sound.shutdown();
  renderer.shutdown();
  if (window) SDL_DestroyWindow(window);
  TTF_Quit(); IMG_Quit(); SDL_Quit();
}

}  // namespace px
