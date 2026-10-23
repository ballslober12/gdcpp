// Scene (display list, tweens, input, camera effects) and App (SDL window, main loop).
#pragma once
#include <SDL.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "audio.h"
#include "objects.h"
#include "particles.h"
#include "renderer.h"
#include "texture.h"
#include "tween.h"

namespace px {

class App;

struct Pointer { double x = 0, y = 0; bool isDown = false; };

class Scene {
 public:
  explicit Scene(App* a) : app(a) {}
  virtual ~Scene();
  App* app;

  virtual void create() {}
  virtual void update(double /*time*/, double /*delta*/) {}
  virtual void onResize() {}
  virtual void onSuspend(bool /*hidden*/) {}

  // --- factories (scene.add.*)
  template <class T, class... A>
  T* make(A&&... a) {
    auto o = std::make_unique<T>(this, std::forward<A>(a)...);
    T* p = o.get();
    all_.push_back(std::move(o));
    displayList.push_back(p);
    return p;
  }
  Image* addImage(double x, double y, const std::string& key, const std::string& frame = "");
  Container* addContainer(double x = 0, double y = 0);
  Rectangle* addRectangle(double x, double y, double w, double h, uint32_t color, double fa = 1);
  TileSprite* addTileSprite(double x, double y, double w, double h, const std::string& key);
  Graphics* addGraphics();
  BitmapText* addBitmapText(double x, double y, const std::string& font, const std::string& text,
                            double size);
  TextObj* addText(double x, double y, const std::string& text, int px_, uint32_t color);
  ParticleEmitter* addParticles(double x, double y, const std::string& texKey,
                                const EmitterConfig& cfg);

  void destroyObject(GameObject* o);
  void registerInteractive(GameObject* o);

  // --- systems
  TweenManager tweens;
  Clock clock;
  std::vector<GameObject*> displayList;
  Pointer pointer;
  std::function<void()> onPointerDown, onPointerUp;
  std::function<void(SDL_Keycode)> onKeyDown;   // non-repeat key presses
  bool keyDown(SDL_Scancode sc) const;

  // --- camera effects
  void cameraShake(double ms, double intensity);
  void cameraFadeIn(double ms);
  void cameraFadeOut(double ms, std::function<void(double)> cb);

  // --- helpers
  Tween* tween(const void* owner, double duration, EaseFn e, std::function<void(double)> upd,
               std::function<void()> done = nullptr, double delay = 0, bool yoyo = false,
               int repeat = 0, std::function<void()> onStart = nullptr);
  // Tween of numeric properties captured at start (Phaser tweens: from current to target).
  Tween* tweenProp(GameObject* o, double* prop, double to, double duration, EaseFn e,
                   std::function<void()> done = nullptr, double delay = 0, bool yoyo = false,
                   int repeat = 0);

  // --- engine internals (called by App)
  void engineUpdate(double time, double delta);
  void render(Renderer& r);
  void handleEvent(const SDL_Event& e);
  bool restartRequested = false;
  double time_ = 0;

 private:
  GameObject* hitTest(double px_, double py_);
  void updateOver();
  Mat worldOf(const GameObject* o, bool* visible) const;
  bool orderAbove(const GameObject* a, const GameObject* b) const;
  void sweep();
  void drawObject(Renderer& r, GameObject* o);

  std::vector<std::unique_ptr<GameObject>> all_;
  std::vector<GameObject*> interactives_;
  GameObject* over_ = nullptr;
  GameObject* dragging_ = nullptr;
  // camera
  double shakeLeft_ = 0, shakeDur_ = 0, shakeInt_ = 0, shakeX_ = 0, shakeY_ = 0;
  double fadeAlpha_ = 0; bool fadeActive_ = false; double fadeDur_ = 0, fadeEl_ = 0; bool fadeOut_ = false;
  std::function<void(double)> fadeCb_;
};

class App {
 public:
  bool init();
  void run();
  void shutdown();
  template <class S> void startScene() { next_ = [this]() { return std::make_unique<S>(this); }; }
  std::function<void(double)> onScreenWidthChanged;

  std::string assetPath(const std::string& rel) const;

  SDL_Window* window = nullptr;
  Renderer renderer;
  std::unique_ptr<TextureManager> textures;
  SoundManager sound;
  std::map<std::string, BitmapFont> fonts;
  std::map<std::string, double> registry;
  std::string levelText;

  double screenW = 1138, screenH = 640;
  double loopDelta = 1000.0 / 60.0;  // Phaser game.loop.delta (smoothed, ms)
  bool fullscreen = false;
  void toggleFullscreen();
  void applyLogicalSize();
  Scene* scene() { return scene_.get(); }
  void requestSceneRestart() { restart_ = true; }
  void present();  // render the current scene immediately (used while loading)

 private:
  std::unique_ptr<Scene> scene_;
  std::function<std::unique_ptr<Scene>()> next_, cur_;
  bool restart_ = false, running_ = true;
  std::string base_;
  double history_[10];
  int histIdx_ = 0;
  friend class Scene;
};

}  // namespace px
