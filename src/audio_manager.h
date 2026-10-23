// Port of audio-manager.js
#pragma once
#include <memory>
#include "engine/scene.h"

class AudioManager {
 public:
  AudioManager(px::Scene* scene, const double* sfxVolume);
  void startMusic();
  void stopMusic();
  void pauseMusic();
  void resumeMusic();
  double getUserMusicVolume() const { return userMusicVol_; }
  void setUserMusicVolume(double v);
  double getMusicVolume() const { return effectiveVolume(); }
  void setMusicVolume(double v) { setUserMusicVolume(v / 0.8); }
  void fadeInMusic(double ms = 1000);
  void fadeOutMusic(double ms = 1500);
  void playEffect(const std::string& key, double volume = 0);  // 0 -> default 1 (JS "||")
  void update(double dt);
  double getMeteringValue() const { return meterValue_; }
  void reset();

 private:
  double effectiveVolume() const { return 0.8 * userMusicVol_; }
  px::Scene* scene_;
  const double* sfxVolume_;
  std::unique_ptr<px::MusicTrack> music_;
  double userMusicVol_;
  bool meteringEnabled_ = false;
  double meterValue_ = 0.1, lastAudio_ = 0.1, lastPeak_ = 0;
  int silenceCounter_ = 0;
};
