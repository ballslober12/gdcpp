// Display objects: a compact stand-in for the Phaser GameObjects the game uses.
#pragma once
#include <SDL.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "math.h"
#include "renderer.h"
#include "texture.h"

namespace px {

class Scene;
class Container;

struct Interactive {
  bool draggable = false;
  double hx = 0, hy = 0, hw = 0, hh = 0;  // hit rect in object space (origin-adjusted)
  std::function<void()> onDown, onUp, onOut;
  std::function<void(double, double)> onDrag;  // dragX, dragY (parent space)
  // drag runtime
  double dragStartX = 0, dragStartY = 0, ptrStartX = 0, ptrStartY = 0;
};

class GameObject {
 public:
  explicit GameObject(Scene* s) : scene(s) {}
  virtual ~GameObject() = default;

  Scene* scene;
  Container* parent = nullptr;
  double x = 0, y = 0, scaleX = 1, scaleY = 1, rotation = 0;
  double alpha = 1;
  bool visible = true;
  double depth = 0;
  double originX = 0.5, originY = 0.5;
  uint32_t tint = 0xffffff;
  Blend blend = Blend::Normal;
  bool dead = false;
  std::unique_ptr<Interactive> input;

  // Level-renderer bookkeeping (dynamic properties in the JS original).
  double worldX = 0;  // `_worldX` on ground tiles
  double eeWorldX = 0, eeBaseY = 0;
  int eeLayer = 1;
  bool eeActive = false, eeBehindParent = false, eeAudioScale = false;
  bool pressed = false;      // `_pressed` on bouncy buttons
  bool glowEnabled = false;  // `_glowEnabled` on player glow layers

  virtual double width() const { return 0; }
  virtual double height() const { return 0; }
  virtual void render(Renderer& r, const Mat& world, double alpha) = 0;

  Mat local() const { return Mat::TRS(x, y, rotation, scaleX, scaleY); }
  bool willRender() const { return visible && alpha != 0 && scaleX * scaleY != 0; }

  GameObject* setPosition(double px_, double py_) { x = px_; y = py_; return this; }
  GameObject* setScale(double s) { scaleX = scaleY = s; return this; }
  GameObject* setScale(double sx, double sy) { scaleX = sx; scaleY = sy; return this; }
  double scale() const { return scaleX; }
  GameObject* setAngle(double deg) { rotation = rad(deg); return this; }
  double angle() const { return rotation * 180.0 / PI; }
  GameObject* setAlpha(double a) { alpha = a; return this; }
  GameObject* setVisible(bool v) { visible = v; return this; }
  GameObject* setDepth(double d) { depth = d; return this; }
  GameObject* setOrigin(double ox, double oy) { originX = ox; originY = oy; return this; }
  GameObject* setOrigin(double o) { originX = originY = o; return this; }
  GameObject* setTint(uint32_t t) { tint = t; return this; }
  GameObject* setBlendMode(Blend b) { blend = b; return this; }

  // Makes the object clickable. Hit rect defaults to (0,0,width,height).
  Interactive* setInteractive();
  void destroy();
};

class Container : public GameObject {
 public:
  using GameObject::GameObject;
  std::vector<GameObject*> list;
  void add(GameObject* o);
  void addAt(GameObject* o, size_t idx);
  void remove(GameObject* o);
  void render(Renderer& r, const Mat& world, double alpha) override;
};

class Image : public GameObject {
 public:
  Image(Scene* s, Texture* t, Frame* f) : GameObject(s), tex(t) { setFrame(f); }
  Texture* tex;
  Frame* frame = nullptr;
  bool flipX = false, flipY = false;
  bool cropped = false;
  double cropX = 0, cropY = 0, cropW = 0, cropH = 0;

  void setFrame(Frame* f);
  void setTexture(Texture* t, Frame* f) { tex = t; setFrame(f); }
  Image* setFlipX(bool v) { flipX = v; return this; }
  Image* setFlipY(bool v) { flipY = v; return this; }
  void setCrop(double cx, double cy, double cw, double ch) {
    cropped = true; cropX = cx; cropY = cy; cropW = cw; cropH = ch;
  }
  void setDisplaySize(double w, double h) {
    scaleX = w / width(); scaleY = h / height();
  }
  double width() const override { return frame ? frame->realW : 0; }
  double height() const override { return frame ? frame->realH : 0; }
  void render(Renderer& r, const Mat& world, double alpha) override;
};

// Draws a frame the way Phaser draws an untinted/tinted sprite quad (used by particles too).
void drawFrameQuad(Renderer& r, const Mat& world, Frame* f, double originX, double originY,
                   bool flipX, bool flipY, uint32_t tint, double alpha, Blend blend);

class Rectangle : public GameObject {
 public:
  Rectangle(Scene* s, double w, double h, uint32_t color, double fa)
      : GameObject(s), w(w), h(h), fillColor(color), fillAlpha(fa) {}
  double w, h;
  uint32_t fillColor;
  double fillAlpha;
  double width() const override { return w; }
  double height() const override { return h; }
  void render(Renderer& r, const Mat& world, double alpha) override;
};

class TileSprite : public GameObject {
 public:
  TileSprite(Scene* s, Texture* t, double w, double h) : GameObject(s), tex(t), w(w), h(h) {}
  Texture* tex;
  double w, h;
  double tilePositionX = 0, tilePositionY = 0;
  double width() const override { return w; }
  double height() const override { return h; }
  void render(Renderer& r, const Mat& world, double alpha) override;
};

class Graphics : public GameObject {
 public:
  explicit Graphics(Scene* s) : GameObject(s) {}
  struct Cmd { std::vector<double> xy; uint32_t color; double alpha; };
  std::vector<Cmd> cmds;
  double lineW = 1; uint32_t lineColor = 0xffffff; double lineAlpha = 1;
  uint32_t fillColor = 0xffffff; double fillAlpha = 1;
  std::vector<double> path;

  Graphics* clear() { cmds.clear(); path.clear(); return this; }
  Graphics* lineStyle(double w, uint32_t c, double a = 1) { lineW = w; lineColor = c; lineAlpha = a; return this; }
  Graphics* fillStyle(uint32_t c, double a = 1) { fillColor = c; fillAlpha = a; return this; }
  Graphics* lineBetween(double x1, double y1, double x2, double y2);
  Graphics* strokeCircle(double cx, double cy, double rad_);
  Graphics* fillCircle(double cx, double cy, double rad_);
  Graphics* strokeRect(double rx, double ry, double rw, double rh);
  Graphics* fillRect(double rx, double ry, double rw, double rh);
  Graphics* beginPath() { path.clear(); return this; }
  Graphics* moveTo(double px_, double py_) { path.clear(); path.push_back(px_); path.push_back(py_); return this; }
  Graphics* lineTo(double px_, double py_) { path.push_back(px_); path.push_back(py_); return this; }
  Graphics* closePath() { return this; }
  Graphics* fillPath();
  void render(Renderer& r, const Mat& world, double alpha) override;
};

struct Glyph {
  double x = 0, y = 0, w = 0, h = 0, xOffset = 0, yOffset = 0, xAdvance = 0;
  std::map<int, double> kerning;
};
struct BitmapFont {
  std::string textureKey;
  double size = 0, lineHeight = 0;
  std::map<int, Glyph> chars;
};

class BitmapText : public GameObject {
 public:
  BitmapText(Scene* s, const BitmapFont* f, Texture* t, const std::string& txt, double fontSize);
  const BitmapFont* font; Texture* tex;
  std::string text;
  double fontSize;
  bool centerAlign = false;
  BitmapText* setText(const std::string& t) { text = t; dirty_ = true; return this; }
  BitmapText* setCenterAlign() { centerAlign = true; dirty_ = true; return this; }
  double width() const override { const_cast<BitmapText*>(this)->layout(); return w_; }
  double height() const override { const_cast<BitmapText*>(this)->layout(); return h_; }
  void render(Renderer& r, const Mat& world, double alpha) override;

 private:
  struct G { double x, y, w, h, u0, v0, u1, v1; int line; };
  void layout();
  bool dirty_ = true;
  double w_ = 0, h_ = 0;
  std::vector<G> glyphs_;
  std::vector<double> lineLen_;
  double longest_ = 0;
};

class TextObj : public GameObject {
 public:
  TextObj(Scene* s, const std::string& txt, int px_, uint32_t color);
  ~TextObj() override;
  std::string text;
  int sizePx; uint32_t color;
  TextObj* setText(const std::string& t);
  double width() const override { return tw_ / 2.0; }
  double height() const override { return th_ / 2.0; }
  void render(Renderer& r, const Mat& world, double alpha) override;

 private:
  void rebuild();
  SDL_Texture* tex_ = nullptr;
  int tw_ = 0, th_ = 0;
};

}  // namespace px

namespace px {
// Path of the TTF used for TextObj (set once by App).
void setUiFontPath(const std::string& path);
}  // namespace px
