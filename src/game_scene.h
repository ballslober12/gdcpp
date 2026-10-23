// Port of game-scene.js
#pragma once
#include <memory>
#include <optional>
#include <vector>
#include "audio_manager.h"
#include "color_manager.h"
#include "constants.h"
#include "engine/scene.h"
#include "level_renderer.h"
#include "player.h"
#include "player_physics_state.h"

class GameScene : public px::Scene {
 public:
  explicit GameScene(px::App* a);
  ~GameScene() override;
  void create() override;
  void update(double time, double delta) override;
  void onResize() override;
  void onSuspend(bool hidden) override;

  void toggleGlitter(bool v);

  // read/written by Player
  double playerWorldX = 0, cameraX = 0, cameraY = 0;

 private:
  struct EndCam { double p = 0, fromX = 0, toX = 0, fromY = 0, toY = 0; };

  void buildHUD();
  void setParticleTimeScale(double v);
  void pauseGame();
  void resumeGame();
  void buildPauseOverlay();
  void buildInfoPopup();
  void closeInfoPopup();
  void expandHitArea(px::GameObject* g, double f);
  void makeBouncyButton(px::GameObject* g, double base, std::function<void()> onClick,
                        std::function<bool()> enabled = nullptr);
  px::Tween* tweenScaleTo(px::GameObject* o, double to, double dur, px::EaseFn e,
                          std::function<void()> done = nullptr);
  void toggleFullscreen();
  px::Container* drawScale9(double x, double y, double w, double h, const std::string& key,
                            double corner, uint32_t tint, double alpha);
  void startGame();
  void pushButton();
  void releaseButton();
  void positionMenuItems();
  void positionAttemptsLabel();
  void resetGameplayState();
  void restartLevel();
  void applyScreenResize();
  void updateBackground();
  void updateCameraY(double dt);
  double quantizeDelta(double deltaMs);
  void showNewBest();
  void triggerEndPortal();
  void levelComplete();
  void showCompleteEffect();
  void showCompleteText();
  void showEndLayer();
  void playStarAward();
  void hideEndLayer(std::function<void()> cb);

  double bgSpeedX_ = 0.1, bgSpeedY_ = 0.1, menuCameraX_ = 0, prevCameraX_ = 0, bgInitY_ = 0;
  px::TileSprite* bg_ = nullptr;
  FloatRef cameraXRef_;
  std::unique_ptr<PlayerPhysicsState> state_;
  std::unique_ptr<LevelRenderer> level_;
  std::unique_ptr<Player> player_;
  std::unique_ptr<ColorManager> colorManager_;
  std::unique_ptr<AudioManager> audio_;
  double glitterCenterX_ = 0, glitterCenterY_ = 0;
  px::ParticleEmitter* glitterEmitter_ = nullptr;
  int attempts_ = 1;
  double bestPercent_ = 0, lastPercent_ = 0, endPortalGameY_ = 240;
  int totalJumps_ = 0;
  double playTime_ = 0;
  bool menuActive_ = true, slideIn_ = false, firstPlay_ = true;
  std::optional<double> slideGroundX_;
  px::Image *logo_ = nullptr, *robLogo_ = nullptr, *tryMeImg_ = nullptr, *menuFsBtn_ = nullptr,
            *menuInfoBtn_ = nullptr, *playBtn_ = nullptr, *pauseBtn_ = nullptr;
  px::TextObj *copyrightText_ = nullptr, *fpsText_ = nullptr;
  std::vector<px::Image*> downloadBtns_;
  px::ParticleEmitter* menuGlitter_ = nullptr;
  bool playBtnPressed_ = false, spaceWasDown_ = false, paused_ = false;
  px::Container *pauseContainer_ = nullptr, *infoPopup_ = nullptr;
  double sfxVolume_ = 1;
  px::BitmapText* attemptsLabel_ = nullptr;
  double fpsAccum_ = 0;
  int fpsFrames_ = 0;
  double deltaBuffer_ = 0, deathTimer_ = 0;
  bool deathSoundPlayed_ = false, newBestShown_ = false, hadNewBest_ = false, levelWon_ = false,
       endCameraOverride_ = false;
  std::shared_ptr<EndCam> endCamTween_;
  px::Rectangle* endLayerOverlay_ = nullptr;
  px::Container* endLayerInternal_ = nullptr;
  double endStarX_ = 0, endStarY_ = 0;
};
