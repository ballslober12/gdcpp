#include "audio_manager.h"
#include <cmath>

AudioManager::AudioManager(px::Scene* scene, const double* sfxVolume)
    : scene_(scene), sfxVolume_(sfxVolume) {
  auto it = scene->app->registry.find("userMusicVol");
  userMusicVol_ = it != scene->app->registry.end() ? it->second : 1;
}

void AudioManager::startMusic() {
  if (music_) music_->stop();
  music_ = scene_->app->sound.addMusic("stereo_madness", true, effectiveVolume());
  music_->play();
  meteringEnabled_ = scene_->app->sound.ready();
}
void AudioManager::stopMusic() { if (music_) music_->stop(); }
void AudioManager::pauseMusic() { if (music_ && music_->isPlaying()) music_->pause(); }
void AudioManager::resumeMusic() { if (music_ && music_->isPaused()) music_->resume(); }

void AudioManager::setUserMusicVolume(double v) {
  userMusicVol_ = v;
  scene_->app->registry["userMusicVol"] = v;
  if (music_) music_->setVolume(effectiveVolume());
}

void AudioManager::fadeInMusic(double ms) {
  if (music_) music_->stop();
  music_ = scene_->app->sound.addMusic("stereo_madness", true, 0);
  music_->play();
  meteringEnabled_ = scene_->app->sound.ready();
  px::MusicTrack* m = music_.get();
  double target = effectiveVolume();
  scene_->tween(this, ms, px::ease::linear, [this, m, target](double p) {
    if (music_.get() == m) m->setVolume(target * p);
  });
}

void AudioManager::fadeOutMusic(double ms) {
  if (!music_ || !music_->isPlaying()) return;
  music_->setLoop(false);
  px::MusicTrack* m = music_.get();
  double from = m->volume();
  scene_->tween(this, ms, px::ease::linear,
                [this, m, from](double p) { if (music_.get() == m) m->setVolume(from * (1 - p)); },
                [this, m]() { if (music_.get() == m) m->stop(); });
}

void AudioManager::playEffect(const std::string& key, double volume) {
  double v = (volume != 0 ? volume : 1) * (sfxVolume_ ? *sfxVolume_ : 1);
  scene_->app->sound.play(key, v);
}

void AudioManager::update(double dt) {
  if (!meteringEnabled_) return;
  double peak = scene_->app->sound.meterPeak();
  double eff = effectiveVolume();
  if (eff > 0) peak /= eff;
  meterValue_ = 0.1 + peak;
  double f = 60 * dt;
  if (silenceCounter_ < 3 || meterValue_ < 1.1 * lastAudio_ ||
      (meterValue_ < 0.95 * lastPeak_ && lastAudio_ > 0.2 * lastPeak_)) {
    meterValue_ = lastAudio_ * std::pow(0.92, f);
  } else {
    silenceCounter_ = 0;
    lastPeak_ = meterValue_;
    meterValue_ *= std::pow(1.46, f);
  }
  if (meterValue_ <= 0.1) lastPeak_ = 0;
  lastAudio_ = meterValue_;
  silenceCounter_++;
}

void AudioManager::reset() {
  meterValue_ = 0.1; lastAudio_ = 0.1; lastPeak_ = 0; silenceCounter_ = 0;
  stopMusic();
}
