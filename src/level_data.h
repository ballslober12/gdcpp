// Port of level-data-helpers.js + sprite-layer-helper.js
#pragma once
#include <string>
#include "engine/scene.h"

enum class ObjKind { Solid, Hazard, PortalFly, PortalCube };

struct LevelObject {
  ObjKind type;
  double x, y, w, h;
  bool activated = false;
  double portalY = 0;  // dynamic `portalY` on portal objects
  LevelObject(ObjKind t, double x_, double y_, double w_, double h_)
      : type(t), x(x_), y(y_), w(w_), h(h_) {}
};

struct AtlasRef { std::string atlas, frame; bool ok = false; };

inline AtlasRef findAtlasFrame(px::Scene* scene, const std::string& frameName) {
  px::Texture* t = scene->app->textures->get("GJ_WebSheet");
  if (t && t->has(frameName)) return {"GJ_WebSheet", frameName, true};
  return {};
}

inline px::Image* addImageFromAtlas(px::Scene* scene, double x, double y,
                                    const std::string& frameName) {
  AtlasRef r = findAtlasFrame(scene, frameName);
  if (r.ok) return scene->addImage(x, y, r.atlas, r.frame);
  if (scene->app->textures->exists(frameName)) return scene->addImage(x, y, frameName);
  return nullptr;
}

// createLayeredSprite: returns the image (the JS returned {sprite}) or nullptr.
inline px::Image* createLayeredSprite(px::Scene* scene, double x, double y,
                                      const std::string& frameName, double depth, bool visible) {
  AtlasRef r = findAtlasFrame(scene, frameName);
  if (!r.ok) return nullptr;
  px::Image* im = scene->addImage(x, y, r.atlas, r.frame);
  im->setDepth(depth);
  im->setVisible(visible);
  return im;
}
