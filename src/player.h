// Port of player.js
#pragma once
#include <memory>
#include <optional>
#include <vector>
#include "constants.h"
#include "engine/scene.h"
#include "level_data.h"
#include "level_renderer.h"
#include "player_physics_state.h"
#include "trail_renderer.h"

class GameScene;

class Player {
 public:
  Player(GameScene* scene, PlayerPhysicsState* state, LevelRenderer* gameLayer);
  ~Player();

  void syncSprites(double cameraX, double cameraY, double dt,
                   std::optional<double> xOverride = std::nullopt);
  void setCubeVisible(bool v);
  void setShipVisible(bool v);
  void enterShipMode(const LevelObject* portal = nullptr);
  void exitShipMode();
  void hitGround();
  void killPlayer();
  void updateExplosionPieces(double deltaMs);
  void runRotateAction();
  void stopRotation();
  void updateRotateAction(double dt);
  void updateGroundRotation(double dt);
  void updateShipRotation(double dt);
  void updateJump(double dt);
  void checkCollisions(double cameraX);
  void drawHitboxes(px::Graphics* g, double cameraX, double cameraY);
  void setShowHitboxes(bool v) { showHitboxes_ = v; }
  void playEndAnimation(double targetX, std::function<void()> done, double portalY = 0);
  void reset();

  PlayerPhysicsState* p;
  bool rotateActionActive = false;
  double rotation_ = 0;

 private:
  void createSprites();
  void initParticles();
  void updateParticles(double cameraX, double cameraY, double dt);
  void createExplosionPieces(double x, double y, double extra);
  void cleanupExplosion();
  void playPortalShine(const LevelObject* pt);
  void checkSnapJump(LevelObject* pt);
  bool isFallingPastThreshold() const;
  double flipMod() const { return p->gravityFlipped ? -1 : 1; }
  double convertToClosestRotation() const;
  static double slerp2D(double a, double b, double t);
  bool playerIsFalling() const;
  void updateFlyJump(double dt);

  GameScene* scene_;
  LevelRenderer* gameLayer_;
  double rotateActionTime_ = 0, rotateActionDuration_ = 0, rotateActionStart_ = 0, rotateActionTotal_ = 0;
  bool showHitboxes_ = false;
  LevelObject* lastLandObject_ = nullptr;
  double lastXOffset_ = 0, lastCameraX_ = 0, lastCameraY_ = 0;
  bool endAnimating_ = false;

  // layers (nullptr when missing)
  px::GameObject *playerSprite_ = nullptr, *playerGlow_ = nullptr, *playerOverlay_ = nullptr,
                 *playerExtra_ = nullptr, *shipSprite_ = nullptr, *shipGlow_ = nullptr,
                 *shipOverlay_ = nullptr, *shipExtra_ = nullptr;
  std::vector<px::GameObject*> playerLayers_, shipLayers_, allLayers_;

  px::ParticleEmitter *particleEmitter_, *flyParticleEmitter_, *flyParticle2Emitter_,
      *shipDragEmitter_, *landEmitter1_, *landEmitter2_;
  px::Container* aboveContainer_;
  bool shipDragActive_ = false, particleActive_ = false, flyParticle2Active_ = false,
       flyParticleActive_ = false, landIdx_ = false;
  std::unique_ptr<TrailRenderer> streak_;

  struct Piece {
    px::Image* spr; px::ParticleEmitter* particle;
    double xVel, yVel, timer, fadeTime, rotDelta, halfSize;
  };
  std::vector<Piece> pieces_;
  bool hasPieces_ = false;
  px::Container* explosionContainer_ = nullptr;
  std::string explosionTexKey_;
  double explosionGroundSY_ = 0;
};
