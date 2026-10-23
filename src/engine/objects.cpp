#include "objects.h"
#include <SDL_ttf.h>
#include <algorithm>
#include "scene.h"

namespace px {

// ---------------------------------------------------------------- GameObject
Interactive* GameObject::setInteractive() {
  input = std::make_unique<Interactive>();
  input->hx = 0; input->hy = 0; input->hw = width(); input->hh = height();
  scene->registerInteractive(this);
  return input.get();
}
void GameObject::destroy() { scene->destroyObject(this); }

// ---------------------------------------------------------------- Container
void Container::add(GameObject* o) {
  if (!o) return;
  if (o->parent) o->parent->remove(o);
  else {
    auto& dl = scene->displayList;
    dl.erase(std::remove(dl.begin(), dl.end(), o), dl.end());
  }
  o->parent = this;
  list.push_back(o);
}
void Container::addAt(GameObject* o, size_t idx) {
  add(o);
  list.pop_back();
  list.insert(list.begin() + std::min(idx, list.size()), o);
}
void Container::remove(GameObject* o) {
  list.erase(std::remove(list.begin(), list.end(), o), list.end());
  if (o->parent == this) o->parent = nullptr;
}
void Container::render(Renderer& r, const Mat& world, double a) {
  for (GameObject* c : list) {
    if (!c || c->dead || !c->willRender()) continue;
    c->render(r, world * c->local(), a * c->alpha);
  }
}

// ---------------------------------------------------------------- Image
void Image::setFrame(Frame* f) { frame = f; }

void drawFrameQuad(Renderer& r, const Mat& world, Frame* f, double ox, double oy, bool flipX,
                   bool flipY, uint32_t tint, double alpha, Blend blend) {
  if (!f || !f->texture) return;
  double dox = ox * f->realW, doy = oy * f->realH;
  double mx = -dox + f->trimX, my = -doy + f->trimY;
  double w = f->width, h = f->height, fx = 1, fy = 1;
  if (flipX) { mx += -f->realW + 2 * dox; fx = -1; }
  if (flipY) { my += -f->realH + 2 * doy; fy = -1; }
  double x0 = mx, x1 = mx + w * fx, y0 = my, y1 = my + h * fy;
  double p[8];
  world.apply(x0, y0, p[0], p[1]);
  world.apply(x1, y0, p[2], p[3]);
  world.apply(x1, y1, p[4], p[5]);
  world.apply(x0, y1, p[6], p[7]);
  r.quad(f->texture->sdl, p, f->u0, f->v0, f->u1, f->v1, tint, alpha, blend);
}

void Image::render(Renderer& r, const Mat& world, double a) {
  if (!frame) return;
  if (!cropped) {
    drawFrameQuad(r, world, frame, originX, originY, flipX, flipY, tint, a, blend);
    return;
  }
  // Frame::setCropUVs
  Frame* f = frame;
  double x = clamp(cropX, 0, f->realW), y = clamp(cropY, 0, f->realH);
  double cw = clamp(cropW, 0, f->realW - x), ch = clamp(cropH, 0, f->realH - y);
  double srcX, srcY, dx, dy, w, h;
  if (f->trimmed) {
    double ssr = f->trimX + f->width, ssb = f->trimY + f->height;
    double cr = x + cw, cb = y + ch;
    if (ssr < x || ssb < y || f->trimX > cr || f->trimY > cb) return;
    double ix = std::max<double>(f->trimX, x), iy = std::max<double>(f->trimY, y);
    w = std::min(ssr, cr) - ix; h = std::min(ssb, cb) - iy;
    srcX = f->cutX + (ix - f->trimX); srcY = f->cutY + (iy - f->trimY);
    dx = ix; dy = iy;
  } else {
    srcX = f->cutX + x; srcY = f->cutY + y; dx = x; dy = y; w = cw; h = ch;
  }
  if (w <= 0 || h <= 0) return;
  double dox = originX * f->realW, doy = originY * f->realH;
  double x0 = -dox + dx, y0 = -doy + dy, x1 = x0 + w, y1 = y0 + h;
  double p[8];
  world.apply(x0, y0, p[0], p[1]);
  world.apply(x1, y0, p[2], p[3]);
  world.apply(x1, y1, p[4], p[5]);
  world.apply(x0, y1, p[6], p[7]);
  Texture* t = f->texture;
  r.quad(t->sdl, p, srcX / t->w, srcY / t->h, (srcX + w) / t->w, (srcY + h) / t->h, tint, a,
         blend);
}

// ---------------------------------------------------------------- Rectangle
void Rectangle::render(Renderer& r, const Mat& world, double a) {
  double x0 = -w * originX, y0 = -h * originY, x1 = x0 + w, y1 = y0 + h;
  double p[12];
  world.apply(x0, y0, p[0], p[1]);
  world.apply(x1, y0, p[2], p[3]);
  world.apply(x1, y1, p[4], p[5]);
  world.apply(x0, y0, p[6], p[7]);
  world.apply(x1, y1, p[8], p[9]);
  world.apply(x0, y1, p[10], p[11]);
  r.triangles(std::vector<double>(p, p + 12), fillColor, fillAlpha * a, blend);
}

// ---------------------------------------------------------------- TileSprite
void TileSprite::render(Renderer& r, const Mat& world, double a) {
  Frame* f = tex->base();
  double tw = f->width, th = f->height;
  if (tw <= 0 || th <= 0 || w <= 0 || h <= 0) return;
  double ox = -w * originX, oy = -h * originY;
  double sx = -std::fmod(tilePositionX, tw);
  if (sx > 0) sx -= tw;
  double sy = -std::fmod(tilePositionY, th);
  if (sy > 0) sy -= th;
  for (double cy = sy; cy < h; cy += th) {
    for (double cx = sx; cx < w; cx += tw) {
      double l = std::max(cx, 0.0), t = std::max(cy, 0.0);
      double rr = std::min(cx + tw, w), bb = std::min(cy + th, h);
      if (rr <= l || bb <= t) continue;
      double u0 = f->u0 + (f->u1 - f->u0) * ((l - cx) / tw);
      double u1 = f->u0 + (f->u1 - f->u0) * ((rr - cx) / tw);
      double v0 = f->v0 + (f->v1 - f->v0) * ((t - cy) / th);
      double v1 = f->v0 + (f->v1 - f->v0) * ((bb - cy) / th);
      double p[8];
      world.apply(ox + l, oy + t, p[0], p[1]);
      world.apply(ox + rr, oy + t, p[2], p[3]);
      world.apply(ox + rr, oy + bb, p[4], p[5]);
      world.apply(ox + l, oy + bb, p[6], p[7]);
      r.quad(tex->sdl, p, u0, v0, u1, v1, tint, a, blend);
    }
  }
}

// ---------------------------------------------------------------- Graphics
Graphics* Graphics::lineBetween(double x1, double y1, double x2, double y2) {
  double dx = x2 - x1, dy = y2 - y1, len = std::sqrt(dx * dx + dy * dy);
  if (len <= 0) return this;
  double nx = -dy / len * lineW / 2, ny = dx / len * lineW / 2;
  Cmd c;
  c.xy = {x1 + nx, y1 + ny, x2 + nx, y2 + ny, x2 - nx, y2 - ny,
          x1 + nx, y1 + ny, x2 - nx, y2 - ny, x1 - nx, y1 - ny};
  c.color = lineColor; c.alpha = lineAlpha;
  cmds.push_back(std::move(c));
  return this;
}
Graphics* Graphics::strokeCircle(double cx, double cy, double rr) {
  const int N = 40;
  double ro = rr + lineW / 2, ri = std::max(0.0, rr - lineW / 2);
  Cmd c; c.color = lineColor; c.alpha = lineAlpha;
  for (int i = 0; i < N; i++) {
    double a0 = 2 * PI * i / N, a1 = 2 * PI * (i + 1) / N;
    double ox0 = cx + std::cos(a0) * ro, oy0 = cy + std::sin(a0) * ro;
    double ox1 = cx + std::cos(a1) * ro, oy1 = cy + std::sin(a1) * ro;
    double ix0 = cx + std::cos(a0) * ri, iy0 = cy + std::sin(a0) * ri;
    double ix1 = cx + std::cos(a1) * ri, iy1 = cy + std::sin(a1) * ri;
    c.xy.insert(c.xy.end(), {ox0, oy0, ox1, oy1, ix1, iy1, ox0, oy0, ix1, iy1, ix0, iy0});
  }
  cmds.push_back(std::move(c));
  return this;
}
Graphics* Graphics::fillCircle(double cx, double cy, double rr) {
  const int N = 40;
  Cmd c; c.color = fillColor; c.alpha = fillAlpha;
  for (int i = 0; i < N; i++) {
    double a0 = 2 * PI * i / N, a1 = 2 * PI * (i + 1) / N;
    c.xy.insert(c.xy.end(), {cx, cy, cx + std::cos(a0) * rr, cy + std::sin(a0) * rr,
                             cx + std::cos(a1) * rr, cy + std::sin(a1) * rr});
  }
  cmds.push_back(std::move(c));
  return this;
}
Graphics* Graphics::fillRect(double rx, double ry, double rw, double rh) {
  Cmd c; c.color = fillColor; c.alpha = fillAlpha;
  c.xy = {rx, ry, rx + rw, ry, rx + rw, ry + rh, rx, ry, rx + rw, ry + rh, rx, ry + rh};
  cmds.push_back(std::move(c));
  return this;
}
Graphics* Graphics::strokeRect(double rx, double ry, double rw, double rh) {
  double h = lineW / 2;
  auto bar = [&](double x, double y, double w, double hh) {
    Cmd c; c.color = lineColor; c.alpha = lineAlpha;
    c.xy = {x, y, x + w, y, x + w, y + hh, x, y, x + w, y + hh, x, y + hh};
    cmds.push_back(std::move(c));
  };
  bar(rx - h, ry - h, rw + lineW, lineW);
  bar(rx - h, ry + rh - h, rw + lineW, lineW);
  bar(rx - h, ry + h, lineW, rh - lineW);
  bar(rx + rw - h, ry + h, lineW, rh - lineW);
  return this;
}
Graphics* Graphics::fillPath() {
  if (path.size() < 6) return this;
  Cmd c; c.color = fillColor; c.alpha = fillAlpha;
  for (size_t i = 2; i + 3 < path.size(); i += 2)
    c.xy.insert(c.xy.end(), {path[0], path[1], path[i], path[i + 1], path[i + 2], path[i + 3]});
  cmds.push_back(std::move(c));
  return this;
}
void Graphics::render(Renderer& r, const Mat& world, double a) {
  for (const Cmd& c : cmds) {
    std::vector<double> t(c.xy.size());
    for (size_t i = 0; i + 1 < c.xy.size(); i += 2) world.apply(c.xy[i], c.xy[i + 1], t[i], t[i + 1]);
    r.triangles(t, c.color, c.alpha * a, blend);
  }
}

// ---------------------------------------------------------------- BitmapText
BitmapText::BitmapText(Scene* s, const BitmapFont* f, Texture* t, const std::string& txt,
                       double fs)
    : GameObject(s), font(f), tex(t), text(txt), fontSize(fs) {
  originX = originY = 0;
}

void BitmapText::layout() {
  if (!dirty_) return;
  dirty_ = false;
  glyphs_.clear(); lineLen_.clear();
  double k = (fontSize ? fontSize : font->size) / font->size;
  double E = 0, A = 0, maxEnd = 0, maxBottom = 0;
  int prev = -1, line = 0;
  auto endLine = [&]() {
    lineLen_.push_back(E * k);
    maxEnd = std::max(maxEnd, E);
  };
  bool any = false;
  for (unsigned char ch : text) {
    int code = ch;
    if (code == 10) {
      endLine(); line++; E = 0; A += font->lineHeight; prev = -1;
      continue;
    }
    auto it = font->chars.find(code);
    if (it == font->chars.end()) continue;
    const Glyph& g = it->second;
    double kern = 0;
    if (prev >= 0) { auto kt = g.kerning.find(prev); if (kt != g.kerning.end()) kern = kt->second; }
    any = true;
    G q;
    q.x = (g.xOffset + E + kern) * k; q.y = (g.yOffset + A) * k;
    q.w = g.w * k; q.h = g.h * k;
    q.u0 = g.x / tex->w; q.v0 = g.y / tex->h;
    q.u1 = (g.x + g.w) / tex->w; q.v1 = (g.y + g.h) / tex->h;
    q.line = line;
    glyphs_.push_back(q);
    E += g.xAdvance + kern;
    maxBottom = std::max(maxBottom, A + font->lineHeight);
    prev = code;
  }
  if (any || !text.empty()) endLine();
  longest_ = 0;
  for (double l : lineLen_) longest_ = std::max(longest_, l);
  w_ = maxEnd * k;
  h_ = maxBottom * k;
}

void BitmapText::render(Renderer& r, const Mat& world, double a) {
  layout();
  double dox = originX * w_, doy = originY * h_;
  for (const G& g : glyphs_) {
    double off = centerAlign ? (longest_ - lineLen_[g.line]) / 2 : 0;
    double x0 = g.x - dox + off, y0 = g.y - doy, x1 = x0 + g.w, y1 = y0 + g.h;
    double p[8];
    world.apply(x0, y0, p[0], p[1]);
    world.apply(x1, y0, p[2], p[3]);
    world.apply(x1, y1, p[4], p[5]);
    world.apply(x0, y1, p[6], p[7]);
    r.quad(tex->sdl, p, g.u0, g.v0, g.u1, g.v1, tint, a, blend);
  }
}

// ---------------------------------------------------------------- TextObj (TTF)
static std::string g_fontPath;
static std::map<int, TTF_Font*> g_fonts;
void setUiFontPath(const std::string& p) { g_fontPath = p; }
static TTF_Font* fontFor(int px_) {
  auto it = g_fonts.find(px_);
  if (it != g_fonts.end()) return it->second;
  TTF_Font* f = TTF_OpenFont(g_fontPath.c_str(), px_);
  g_fonts[px_] = f;
  return f;
}

TextObj::TextObj(Scene* s, const std::string& txt, int px_, uint32_t col)
    : GameObject(s), text(txt), sizePx(px_), color(col) {
  originX = originY = 0;
  rebuild();
}
TextObj::~TextObj() { if (tex_) SDL_DestroyTexture(tex_); }
TextObj* TextObj::setText(const std::string& t) { if (t != text) { text = t; rebuild(); } return this; }

void TextObj::rebuild() {
  if (tex_) { SDL_DestroyTexture(tex_); tex_ = nullptr; }
  tw_ = th_ = 0;
  TTF_Font* f = fontFor(sizePx * 2);  // resolution 2
  if (!f || text.empty()) return;
  SDL_Color c = {(Uint8)(color >> 16), (Uint8)(color >> 8), (Uint8)color, 255};
  SDL_Surface* sf = TTF_RenderUTF8_Blended_Wrapped(f, text.c_str(), c, 0);
  if (!sf) return;
  tex_ = SDL_CreateTextureFromSurface(scene->app->renderer.sdl(), sf);
  tw_ = sf->w; th_ = sf->h;
  SDL_FreeSurface(sf);
  if (tex_) SDL_SetTextureBlendMode(tex_, SDL_BLENDMODE_BLEND);
}

void TextObj::render(Renderer& r, const Mat& world, double a) {
  if (!tex_) return;
  double w = tw_ / 2.0, h = th_ / 2.0;
  double x0 = -originX * w, y0 = -originY * h, x1 = x0 + w, y1 = y0 + h;
  double p[8];
  world.apply(x0, y0, p[0], p[1]);
  world.apply(x1, y0, p[2], p[3]);
  world.apply(x1, y1, p[4], p[5]);
  world.apply(x0, y1, p[6], p[7]);
  r.quad(tex_, p, 0, 0, 1, 1, 0xffffff, a, blend);
}

}  // namespace px
