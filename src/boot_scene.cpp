#include "boot_scene.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "game_scene.h"

using namespace px;

static bool readFile(const std::string& path, std::string& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) { SDL_Log("loaderror: %s", path.c_str()); return false; }
  std::stringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

void BootScene::create() {
  double width = app->screenW, height = app->screenH;
  Rectangle* bar = addRectangle(width / 2, height / 2, 0.6 * width, 8, 65280);
  bar->scaleX = 0;

  std::string fntBig, fntGold;
  struct Job { std::function<void()> run; };
  std::vector<std::function<void()>> jobs;
  auto path = [this](const std::string& f) { return app->assetPath(f); };
  jobs.push_back([&] { app->textures->loadAtlas("GJ_WebSheet", path("GJ_WebSheet.png"), path("GJ_WebSheet.json")); });
  jobs.push_back([&] { app->textures->loadImage("bigFont", path("bigFont.png")); });
  jobs.push_back([&] { readFile(path("bigFont.fnt"), fntBig); });
  jobs.push_back([&] { app->textures->loadImage("goldFont", path("goldFont.png")); });
  jobs.push_back([&] { readFile(path("goldFont.fnt"), fntGold); });
  jobs.push_back([&] { app->textures->loadImage("game_bg_01", path("game_bg_01_001.png")); });
  jobs.push_back([&] { app->textures->loadImage("sliderBar", path("sliderBar.png")); });
  jobs.push_back([&] { app->textures->loadImage("square04_001", path("square04_001.png")); });
  jobs.push_back([&] { app->textures->loadImage("GJ_square02", path("GJ_square02.png")); });
  jobs.push_back([&] { readFile(path("1.txt"), app->levelText); });
  jobs.push_back([&] { app->sound.loadMusic("stereo_madness", path("StereoMadness.mp3")); });
  for (const char* k : {"explode_11", "endStart_02", "playSound_01", "quitSound_01", "highscoreGet02"})
    jobs.push_back([this, k, path] { app->sound.loadChunk(k, path(std::string(k) + ".ogg")); });
  for (size_t i = 0; i < jobs.size(); i++) {
    jobs[i]();
    bar->scaleX = (double)(i + 1) / jobs.size();
    app->present();
  }
  if (!fntBig.empty()) defineFontFromFnt("bigFont", fntBig);
  if (!fntGold.empty()) defineFontFromFnt("goldFont", fntGold);
  app->startScene<GameScene>();
}

void BootScene::defineFontFromFnt(const std::string& key, const std::string& fnt) {
  BitmapFont font;
  font.textureKey = key;
  struct Kern { int first, second; double amount; };
  std::vector<Kern> kerns;
  std::istringstream lines(fnt);
  std::string line;
  while (std::getline(lines, line)) {
    std::istringstream toks(line);
    std::string head;
    if (!(toks >> head)) continue;
    std::map<std::string, std::string> kv;
    std::string t;
    while (toks >> t) {
      size_t eq = t.find('=');
      if (eq == std::string::npos) continue;
      std::string v = t.substr(eq + 1);
      if (!v.empty() && v.front() == '"') v.erase(0, 1);
      if (!v.empty() && v.back() == '"') v.pop_back();
      kv[t.substr(0, eq)] = v;
    }
    auto num = [&](const char* k) { return (double)std::strtol(kv[k].c_str(), nullptr, 10); };
    if (head == "info") font.size = num("size");
    else if (head == "common") font.lineHeight = num("lineHeight");
    else if (head == "char") {
      Glyph g;
      g.x = num("x"); g.y = num("y"); g.w = num("width"); g.h = num("height");
      g.xOffset = num("xoffset"); g.yOffset = num("yoffset"); g.xAdvance = num("xadvance");
      font.chars[(int)num("id")] = g;
    } else if (head == "kerning") {
      kerns.push_back({(int)num("first"), (int)num("second"), num("amount")});
    }
  }
  for (const Kern& k : kerns) {
    auto it = font.chars.find(k.second);
    if (it != font.chars.end()) it->second.kerning[k.first] = k.amount;
  }
  app->fonts[key] = font;
}
