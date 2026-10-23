// Port of boot-scene.js (+ font-helpers.js)
#pragma once
#include "engine/scene.h"

class BootScene : public px::Scene {
 public:
  explicit BootScene(px::App* a) : Scene(a) {}
  void create() override;

 private:
  void defineFontFromFnt(const std::string& key, const std::string& fnt);
};
