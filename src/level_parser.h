// Port of helperFn16/17/18 (level string parsing) and the object definition table.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

enum class DefType { Solid, Deco, Hazard, Portal, Pad, Ring, Trigger, Speed };

struct ChildDef {
  std::string frame;
  bool hasTint = false; uint32_t tint = 0;
  bool additive = false;
  bool hasZ = false; double z = 0;
  bool audioScale = false;
  bool hasLocalDy = false; double localDy = 0;
};

struct ObjDef {
  DefType type = DefType::Deco;
  bool hasFrame = false; std::string frame;
  double gridW = 0, gridH = 0;
  bool hasSprite = false; double spriteW = 0, spriteH = 0;
  bool hasHitbox = false; double hitboxScaleX = 0, hitboxScaleY = 0;
  bool glow = false, black = false;
  std::vector<std::string> randomFrames;
  std::string sub;
  bool portalParticle = false; uint32_t portalParticleColor = 0;
  bool hasZ = false; double z = 0;
  bool additive = false;
  bool hasTint = false; uint32_t tint = 0;
  int enterEffect = 0;
  std::vector<ChildDef> children;
};

struct LevelItem {
  int id = 0;
  double x = 0, y = 0, rot = 0, scale = 1;
  bool flipX = false, flipY = false;
  int zLayer = 0, zOrder = 0;
  std::string groups;
  int color1 = 0, color2 = 0;
  std::map<int, std::string> raw;
  // _raw lookup helpers (JS `raw[k] ?? def`)
  bool has(int k) const { return raw.count(k) > 0; }
};

void buildObjectTable(std::unordered_map<int, ObjDef>& t);
const ObjDef* getObjDef(int id);                       // helperFn18
bool parseLevelString(const std::string& levelText, std::vector<LevelItem>& out);  // helperFn17
