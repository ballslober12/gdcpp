// Batching quad/triangle renderer on top of SDL_Renderer (SDL_RenderGeometry).
#pragma once
#include <SDL.h>
#include <cstdint>
#include <vector>

namespace px {

enum class Blend { Normal, Add, DstColor };

class Renderer {
 public:
  bool init(SDL_Renderer* r);
  void shutdown();
  void beginFrame();
  void endFrame();
  void flush();

  // Textured quad. p = x0,y0,x1,y1,x2,y2,x3,y3 (tl,tr,br,bl) in logical coords.
  void quad(SDL_Texture* tex, const double* p, double u0, double v0, double u1, double v1,
            uint32_t tint, double alpha, Blend blend);
  // Flat coloured triangle list (positions in logical coords).
  void triangles(const std::vector<double>& xy, uint32_t color, double alpha, Blend blend);
  void fillRect(double x, double y, double w, double h, uint32_t color, double alpha);

  // Render-target switching (used for the death explosion texture).
  void setTarget(SDL_Texture* t, bool clear);

  void setOffset(double x, double y) { offX_ = x; offY_ = y; }
  SDL_Renderer* sdl() { return r_; }

 private:
  void setState(SDL_Texture* tex, Blend b);
  SDL_Renderer* r_ = nullptr;
  SDL_Texture* white_ = nullptr;
  SDL_Texture* curTex_ = nullptr;
  Blend curBlend_ = Blend::Normal;
  std::vector<SDL_Vertex> verts_;
  std::vector<int> idx_;
  double offX_ = 0, offY_ = 0;
};

}  // namespace px
