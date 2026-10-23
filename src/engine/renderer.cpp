#include "renderer.h"

namespace px {

bool Renderer::init(SDL_Renderer* r) {
  r_ = r;
  white_ = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
  if (!white_) return false;
  Uint32 px = 0xffffffff;
  SDL_UpdateTexture(white_, nullptr, &px, 4);
  verts_.reserve(1 << 16);
  idx_.reserve(1 << 17);
  return true;
}

void Renderer::shutdown() {
  if (white_) SDL_DestroyTexture(white_);
  white_ = nullptr;
  if (r_) SDL_DestroyRenderer(r_);
  r_ = nullptr;
}

void Renderer::beginFrame() {
  SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
  SDL_RenderClear(r_);
}
void Renderer::endFrame() {
  flush();
  SDL_RenderPresent(r_);
}

static SDL_BlendMode modeFor(Blend b) {
  switch (b) {
    case Blend::Add:  // Phaser ADD = (SRC_ALPHA, ONE)
      return SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE,
                                        SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO,
                                        SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
    case Blend::DstColor:  // (DST_COLOR, ONE_MINUS_SRC_ALPHA)
      return SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR,
                                        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                        SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE,
                                        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                        SDL_BLENDOPERATION_ADD);
    default:
      return SDL_BLENDMODE_BLEND;
  }
}

void Renderer::flush() {
  if (verts_.empty()) return;
  SDL_SetTextureBlendMode(curTex_, modeFor(curBlend_));
  SDL_RenderGeometry(r_, curTex_, verts_.data(), (int)verts_.size(), idx_.data(),
                     (int)idx_.size());
  verts_.clear();
  idx_.clear();
}

void Renderer::setState(SDL_Texture* tex, Blend b) {
  if (tex != curTex_ || b != curBlend_ || verts_.size() > 60000) {
    flush();
    curTex_ = tex;
    curBlend_ = b;
  }
}

static SDL_Color col(uint32_t tint, double alpha) {
  SDL_Color c;
  c.r = (Uint8)((tint >> 16) & 255);
  c.g = (Uint8)((tint >> 8) & 255);
  c.b = (Uint8)(tint & 255);
  double a = alpha < 0 ? 0 : (alpha > 1 ? 1 : alpha);
  c.a = (Uint8)(a * 255.0 + 0.5);
  return c;
}

void Renderer::quad(SDL_Texture* tex, const double* p, double u0, double v0, double u1,
                    double v1, uint32_t tint, double alpha, Blend blend) {
  if (alpha <= 0) return;
  setState(tex, blend);
  SDL_Color c = col(tint, alpha);
  int base = (int)verts_.size();
  const double us[4] = {u0, u1, u1, u0};
  const double vs[4] = {v0, v0, v1, v1};
  for (int i = 0; i < 4; i++) {
    SDL_Vertex v;
    v.position.x = (float)(p[i * 2] + offX_);
    v.position.y = (float)(p[i * 2 + 1] + offY_);
    v.color = c;
    v.tex_coord.x = (float)us[i];
    v.tex_coord.y = (float)vs[i];
    verts_.push_back(v);
  }
  int ids[6] = {0, 1, 2, 0, 2, 3};
  for (int i : ids) idx_.push_back(base + i);
}

void Renderer::triangles(const std::vector<double>& xy, uint32_t color, double alpha,
                         Blend blend) {
  if (alpha <= 0 || xy.size() < 6) return;
  setState(white_, blend);
  SDL_Color c = col(color, alpha);
  int base = (int)verts_.size();
  for (size_t i = 0; i + 1 < xy.size(); i += 2) {
    SDL_Vertex v;
    v.position.x = (float)(xy[i] + offX_);
    v.position.y = (float)(xy[i + 1] + offY_);
    v.color = c;
    v.tex_coord.x = 0.5f;
    v.tex_coord.y = 0.5f;
    verts_.push_back(v);
  }
  for (size_t i = 0; i < xy.size() / 2; i++) idx_.push_back(base + (int)i);
}

void Renderer::fillRect(double x, double y, double w, double h, uint32_t color, double alpha) {
  triangles({x, y, x + w, y, x + w, y + h, x, y, x + w, y + h, x, y + h}, color, alpha,
            Blend::Normal);
}

void Renderer::setTarget(SDL_Texture* t, bool clear) {
  flush();
  SDL_SetRenderTarget(r_, t);
  if (t && clear) {
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 0);
    SDL_RenderClear(r_);
  }
}

}  // namespace px
