// Port of level-renderer.js
#pragma once
#include <optional>
#include <string>
#include <vector>
#include "color_manager.h"
#include "constants.h"
#include "engine/scene.h"
#include "level_data.h"
#include "level_parser.h"

struct FloatRef { double value = 0; };

struct ColorTrigger { double x; int index; RGB color; double duration; bool tintGround; };
struct EnterEffectTrigger { double x; int effect; };

class LevelRenderer {
 public:
  LevelRenderer(px::Scene* scene, FloatRef* cameraXRef);

  void loadLevel(const std::string& levelText);
  void resizeScreen();
  void updateGroundTiles(double yOffset = 0);
  void shiftGroundTiles(double d);
  void resetGroundTiles(double startX);
  void resetGroundState();
  void setFlyMode(bool on, double playerY);
  void stepGroundAnimation(double dt);
  double getFloorY() const { return flyGroundActive_ ? flyFloorY_ : 0; }
  std::optional<double> getCeilingY() const {
    if (flyGroundActive_) return flyCeilingY_;
    return std::nullopt;
  }
  void createEndPortal(px::Scene* scene);
  void updateEndPortalY(double a, bool other);
  std::vector<ColorTrigger> checkColorTriggers(double x);
  void resetColorTriggers() { colorTriggerIdx_ = 0; }
  void updateVisibility(double cameraX);
  const std::vector<LevelObject*>& getNearbySectionObjects(double x);
  void checkEnterEffectTriggers(double x);
  void resetEnterEffectTriggers();
  void applyEnterEffects(double cameraX);
  void setGroundColor(uint32_t c);
  void updateAudioScale(double s);
  void resetVisibility() { visMinSec_ = visMaxSec_ = -1; }
  void resetObjects();

  px::Container* additiveContainer;
  px::Container* container;
  px::Container* topContainer;
  std::vector<std::unique_ptr<LevelObject>> objects;
  double endXPos = 0;
  std::optional<double> flyCameraTarget;
  const double* cameraYRef = nullptr;  // scene._cameraY
  bool flyGroundActive_ = false;
  double groundTargetValue_ = 0;
  double endPortalGameY_ = 240;
  px::Container* endPortalContainer_ = nullptr;
  double flyFloorY_ = 0, flyCeilingY_ = 0;

 private:
  struct Section { px::Container* additive; px::Container* normal; };
  void buildGround();
  void applyVisualProps(px::Image* g, const std::string& frame, const LevelItem& item,
                        const bool* hasTint, uint32_t tint, bool black);
  void addVisualSprite(px::GameObject* g, const ObjDef* def, bool portalFront);
  static std::string getGlowFrameName(const std::string& f, bool& ok);
  void addGlowSprite(px::Scene* scene, double x, double y, const std::string& frame,
                     const LevelItem& item, double worldX);
  void spawnLevelObjects(const std::vector<LevelItem>& items);
  void addToSection(px::GameObject* o);
  void addCollisionToSection(LevelObject* o);
  void setSectionVisible(int i, bool v);
  void computeFlyBounds(double y, double& floorY, double& ceilY) const;

  px::Scene* scene_;
  FloatRef* cameraXRef_;
  double groundY_ = 0;
  bool groundAnimating_ = false;
  double groundAnimFrom_ = 0, groundAnimTo_ = 0, groundAnimTime_ = 0, groundAnimDuration_ = 0;
  std::vector<ColorTrigger> colorTriggers_;
  size_t colorTriggerIdx_ = 0;
  std::vector<px::GameObject*> audioScaleSprites_;
  std::vector<EnterEffectTrigger> enterEffectTriggers_;
  size_t enterEffectTriggerIdx_ = 0;
  int activeEnterEffect_ = 0, activeExitEffect_ = 0;
  std::vector<std::vector<px::GameObject*>> sections_;
  std::vector<std::unique_ptr<Section>> sectionContainers_;
  std::vector<std::vector<LevelObject*>> collisionSections_;
  std::vector<LevelObject*> nearbyBuffer_;
  int visMinSec_ = -1, visMaxSec_ = -1;
  double groundStartScreenY_, ceilingStartScreenY_ = 0;
  double lastObjectX_ = 0;
  double tileW_ = 1012, maxGroundWorldX_ = 0;
  std::vector<px::Image*> groundTiles_, ceilingTiles_;
  px::Image *groundLine_, *ceilingLine_, *groundShadowL_, *groundShadowR_, *ceilingShadowL_,
      *ceilingShadowR_;
  px::Image* endPortalShine_ = nullptr;
  px::ParticleEmitter* endPortalEmitter_ = nullptr;
};
