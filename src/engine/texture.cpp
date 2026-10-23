#include "texture.h"
#include <SDL_image.h>
#include <fstream>
#include <sstream>
#include "json.h"

namespace px {

Texture::~Texture() {
  if (sdl) SDL_DestroyTexture(sdl);
}

Frame* Texture::get(const std::string& n) {
  auto it = frames.find(n);
  return it == frames.end() ? nullptr : it->second.get();
}

Frame* Texture::add(const std::string& n, int x, int y, int fw, int fh) {
  auto f = std::make_unique<Frame>();
  f->texture = this; f->name = n;
  f->cutX = x; f->cutY = y; f->width = fw; f->height = fh;
  f->realW = fw; f->realH = fh;
  f->u0 = (float)x / w; f->v0 = (float)y / h;
  f->u1 = (float)(x + fw) / w; f->v1 = (float)(y + fh) / h;
  Frame* raw = f.get();
  frames[n] = std::move(f);
  return raw;
}

Texture* TextureManager::get(const std::string& key) {
  auto it = tex_.find(key);
  return it == tex_.end() ? nullptr : it->second.get();
}

Texture* TextureManager::addTexture(const std::string& key, SDL_Texture* t, int w, int h) {
  auto tx = std::make_unique<Texture>();
  tx->key = key; tx->sdl = t; tx->w = w; tx->h = h;
  tx->add("__BASE", 0, 0, w, h);
  Texture* raw = tx.get();
  tex_[key] = std::move(tx);
  return raw;
}

bool TextureManager::loadImage(const std::string& key, const std::string& path) {
  SDL_Texture* t = IMG_LoadTexture(r_, path.c_str());
  if (!t) { SDL_Log("loaderror: %s (%s)", path.c_str(), IMG_GetError()); return false; }
  int w, h;
  SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
  SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
  addTexture(key, t, w, h);
  return true;
}

bool TextureManager::loadAtlas(const std::string& key, const std::string& imagePath,
                               const std::string& jsonPath) {
  if (!loadImage(key, imagePath)) return false;
  std::ifstream in(jsonPath, std::ios::binary);
  if (!in) { SDL_Log("loaderror: %s", jsonPath.c_str()); return false; }
  std::stringstream ss; ss << in.rdbuf();
  Json j = Json::parse(ss.str());
  Texture* t = get(key);
  for (const Json& tx : j["textures"].arr) {
    for (const Json& fr : tx["frames"].arr) {
      const Json& f = fr["frame"];
      Frame* nf = t->add(fr["filename"].str, (int)f["x"].num, (int)f["y"].num,
                         (int)f["w"].num, (int)f["h"].num);
      if (fr["trimmed"].b) {  // Frame::setTrim
        const Json& ss2 = fr["spriteSourceSize"];
        const Json& src = fr["sourceSize"];
        nf->trimmed = true;
        nf->realW = (int)src["w"].num; nf->realH = (int)src["h"].num;
        nf->trimX = (int)ss2["x"].num; nf->trimY = (int)ss2["y"].num;
        nf->width = (int)ss2["w"].num; nf->height = (int)ss2["h"].num;
      }
    }
  }
  return true;
}

}  // namespace px
