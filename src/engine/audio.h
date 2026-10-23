// SDL_mixer wrapper that mimics the small slice of Phaser's sound API the game uses.
#pragma once
#include <SDL.h>
#include <SDL_mixer.h>
#include <atomic>
#include <map>
#include <memory>
#include <string>

namespace px {

class SoundManager;

// Phaser BaseSound equivalent for the looping music track.
class MusicTrack {
 public:
  MusicTrack(SoundManager* m, Mix_Music* mus, bool loop, double vol)
      : mgr_(m), music_(mus), loop_(loop), volume_(vol) {}
  ~MusicTrack();
  void play();
  void stop();
  void pause();
  void resume();
  void setVolume(double v);
  double volume() const { return volume_; }
  bool isPlaying() const { return playing_; }
  bool isPaused() const { return paused_; }
  void setLoop(bool l) { loop_ = l; }

 private:
  SoundManager* mgr_;
  Mix_Music* music_;
  bool loop_;
  double volume_;
  bool playing_ = false, paused_ = false;
};

class SoundManager {
 public:
  bool init();
  void shutdown();
  bool loadChunk(const std::string& key, const std::string& path);
  bool loadMusic(const std::string& key, const std::string& path);
  void play(const std::string& key, double volume);  // one-shot effect
  std::unique_ptr<MusicTrack> addMusic(const std::string& key, bool loop, double volume);
  void suspend(bool s);  // window blur / hidden
  // Peak of the most recent 2048 mono samples (like AnalyserNode time-domain data).
  double meterPeak() const;
  bool ready() const { return ready_; }

 private:
  static void postMix(void* udata, Uint8* stream, int len);
  bool ready_ = false;
  std::map<std::string, Mix_Chunk*> chunks_;
  std::map<std::string, Mix_Music*> music_;
  static constexpr int N = 2048;
  float ring_[N] = {0};
  std::atomic<int> pos_{0};
};

}  // namespace px
