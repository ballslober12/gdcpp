#include "audio.h"
#include <cmath>

namespace px {

MusicTrack::~MusicTrack() { stop(); }

void MusicTrack::setVolume(double v) {
  volume_ = v < 0 ? 0 : (v > 1 ? 1 : v);
  if (playing_ || paused_) Mix_VolumeMusic((int)std::lround(volume_ * MIX_MAX_VOLUME));
}
void MusicTrack::play() {
  if (!music_) return;
  Mix_VolumeMusic((int)std::lround(volume_ * MIX_MAX_VOLUME));
  Mix_PlayMusic(music_, loop_ ? -1 : 1);
  playing_ = true; paused_ = false;
}
void MusicTrack::stop() {
  if (playing_ || paused_) Mix_HaltMusic();
  playing_ = false; paused_ = false;
}
void MusicTrack::pause() {
  if (!playing_) return;
  Mix_PauseMusic();
  playing_ = false; paused_ = true;
}
void MusicTrack::resume() {
  if (!paused_) return;
  Mix_ResumeMusic();
  playing_ = true; paused_ = false;
}

bool SoundManager::init() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
    SDL_Log("audio disabled: %s", Mix_GetError());
    return false;
  }
  Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3);
  Mix_AllocateChannels(32);
  Mix_SetPostMix(&SoundManager::postMix, this);
  ready_ = true;
  return true;
}

void SoundManager::shutdown() {
  if (!ready_) return;
  Mix_HaltMusic();
  Mix_HaltChannel(-1);
  for (auto& c : chunks_) Mix_FreeChunk(c.second);
  for (auto& m : music_) Mix_FreeMusic(m.second);
  chunks_.clear(); music_.clear();
  Mix_CloseAudio();
  ready_ = false;
}

bool SoundManager::loadChunk(const std::string& key, const std::string& path) {
  if (!ready_) return false;
  Mix_Chunk* c = Mix_LoadWAV(path.c_str());
  if (!c) { SDL_Log("loaderror: %s (%s)", path.c_str(), Mix_GetError()); return false; }
  chunks_[key] = c;
  return true;
}
bool SoundManager::loadMusic(const std::string& key, const std::string& path) {
  if (!ready_) return false;
  Mix_Music* m = Mix_LoadMUS(path.c_str());
  if (!m) { SDL_Log("loaderror: %s (%s)", path.c_str(), Mix_GetError()); return false; }
  music_[key] = m;
  return true;
}

void SoundManager::play(const std::string& key, double volume) {
  if (!ready_) return;
  auto it = chunks_.find(key);
  if (it == chunks_.end()) return;
  int ch = Mix_PlayChannel(-1, it->second, 0);
  if (ch >= 0) {
    double v = volume < 0 ? 0 : (volume > 1 ? 1 : volume);
    Mix_Volume(ch, (int)std::lround(v * MIX_MAX_VOLUME));
  }
}

std::unique_ptr<MusicTrack> SoundManager::addMusic(const std::string& key, bool loop,
                                                   double volume) {
  auto it = music_.find(key);
  return std::make_unique<MusicTrack>(this, it == music_.end() ? nullptr : it->second, loop,
                                      volume);
}

void SoundManager::suspend(bool s) {
  if (!ready_) return;
  // Music pause/resume is driven by the game scene (visibilitychange handler); only SFX here.
  if (s) Mix_Pause(-1);
  else Mix_Resume(-1);
}

void SoundManager::postMix(void* udata, Uint8* stream, int len) {
  auto* self = static_cast<SoundManager*>(udata);
  const Sint16* s = reinterpret_cast<const Sint16*>(stream);
  int frames = len / 4;  // stereo S16
  int p = self->pos_.load(std::memory_order_relaxed);
  for (int i = 0; i < frames; i++) {
    self->ring_[p] = ((s[i * 2] + s[i * 2 + 1]) * 0.5f) / 32768.0f;
    p = (p + 1) % N;
  }
  self->pos_.store(p, std::memory_order_relaxed);
}

double SoundManager::meterPeak() const {
  double peak = 0;
  for (int i = 0; i < N; i++) peak = std::max(peak, (double)std::fabs(ring_[i]));
  return peak;
}

}  // namespace px
