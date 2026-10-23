// Texture / atlas frame model that reproduces Phaser's Frame trim semantics.
#pragma once
#include <SDL.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace px {

struct Texture;

struct Frame {
  Texture* texture = nullptr;
  std::string name;
  int cutX = 0, cutY = 0;      // source rect in the texture
  int width = 0, height = 0;   // cut size (Phaser frame.width/height)
  int realW = 0, realH = 0;    // untrimmed size (Phaser frame.realWidth/realHeight)
  bool trimmed = false;
  int trimX = 0, trimY = 0;    // spriteSourceSize x,y (Phaser frame.x/y)
  float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
};

struct Texture {
  std::string key;
  SDL_Texture* sdl = nullptr;
  int w = 0, h = 0;
  std::map<std::string, std::unique_ptr<Frame>> frames;
  ~Texture();
  bool has(const std::string& n) const { return frames.count(n) > 0; }
  Frame* get(const std::string& n);                       // nullptr if missing
  Frame* base() { return get("__BASE"); }
  Frame* add(const std::string& n, int x, int y, int fw, int fh);
};

class TextureManager {
 public:
  explicit TextureManager(SDL_Renderer* r) : r_(r) {}
  bool loadImage(const std::string& key, const std::string& path);
  bool loadAtlas(const std::string& key, const std::string& imagePath,
                 const std::string& jsonPath);
  Texture* addTexture(const std::string& key, SDL_Texture* t, int w, int h);
  bool exists(const std::string& key) const { return tex_.count(key) > 0; }
  Texture* get(const std::string& key);
  void remove(const std::string& key) { tex_.erase(key); }
  SDL_Renderer* sdl() { return r_; }

 private:
  SDL_Renderer* r_;
  std::unordered_map<std::string, std::unique_ptr<Texture>> tex_;
};

}  // namespace px
