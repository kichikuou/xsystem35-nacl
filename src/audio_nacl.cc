#include "config.h"

extern "C" {
#include "audio.h"
#include "portab.h"
}
#include "naclmsg.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ppapi/cpp/audio.h>

class NaclAudio {
public:
  NaclAudio() : ppaudio_(NULL) {}
  ~NaclAudio() { close(NULL); }
  int open(audiodevice_t* audio, chanfmt_t fmt);
  int close(audiodevice_t* audio);
  int write(audiodevice_t* audio, unsigned char* buf, int cnt);
  bool writable() { return !full_; }
  void callback(void* samples, uint32_t buffer_size);
private:
  uint32_t sample_frame_count_;
  pp::Audio* ppaudio_;
  void* buf_;
  pthread_cond_t *cond_;
  bool full_;
};

void AudioCallback(void* samples, uint32_t buffer_size, void* data) {
  static_cast<NaclAudio*>(data)->callback(samples, buffer_size);
}

int NaclAudio::open(audiodevice_t* audio, chanfmt_t fmt) {
  if (ppaudio_)
    close(audio);

  pp::Instance* instance = g_naclMsg->instance();
  PP_AudioSampleRate sample_rate = pp::AudioConfig::RecommendSampleRate(instance);

  if (!(sample_rate == PP_AUDIOSAMPLERATE_44100 && fmt.rate == 44100) ||
      (sample_rate == PP_AUDIOSAMPLERATE_48000 && fmt.rate == 48000))
    return NG;

  sample_frame_count_ = pp::AudioConfig::RecommendSampleFrameCount(instance, sample_rate, 4096);
  if (!sample_frame_count_)
    return NG;

  ppaudio_ = new pp::Audio(instance,
                           pp::AudioConfig(instance, sample_rate, sample_frame_count_),
                           AudioCallback,
                           this);
  audio->buf.len = sample_frame_count_ * 4;  // 16 bits, stereo
  buf_ = malloc(audio->buf.len);
  full_ = false;
  cond_ = audio->pcm_cond;
  return OK;
}

int NaclAudio::close(audiodevice_t* audio) {
  if (ppaudio_) {
    // FIXME: Wait for finish?
    delete ppaudio_;
    ppaudio_ = NULL;
    free(buf_);
  }
  return OK;
}

int NaclAudio::write(audiodevice_t* audio, unsigned char* buf, int cnt) {
  if (!ppaudio_ || full_)
    return NG;

  if (!cnt)
    memset(buf_, 0, audio->buf.len);
  else {
    assert(cnt == audio->buf.len);
    memcpy(buf_, buf, cnt);
  }
  full_ = true;
  ppaudio_->StartPlayback();

  return OK;
}

void NaclAudio::callback(void* samples, uint32_t buffer_size) {
  assert(buffer_size >= sample_frame_count_ * 4);
  memcpy(samples, buf_, sample_frame_count_ * 4);
  full_ = false;
  pthread_cond_signal(cond_);
}

int audio_open(audiodevice_t* audio, chanfmt_t fmt) {
  return static_cast<NaclAudio*>(audio->data_pcm)->open(audio, fmt);
}

int audio_close(audiodevice_t* audio) {
  return static_cast<NaclAudio*>(audio->data_pcm)->close(audio);
}

int audio_write(audiodevice_t* audio, unsigned char* buf, int cnt) {
  return static_cast<NaclAudio*>(audio->data_pcm)->write(audio, buf, cnt);
}

boolean audio_writable(audiodevice_t* audio) {
  return static_cast<NaclAudio*>(audio->data_pcm)->writable() ? TRUE : FALSE;
}

int nacl_audio_exit(audiodevice_t* dev) {
  if (dev == NULL) return OK;
	
  // mixer_exit(dev);
	
  delete static_cast<NaclAudio*>(dev->data_pcm);

  return OK;
}

extern "C" int nacl_audio_init(audiodevice_t* dev);

int nacl_audio_init(audiodevice_t* dev) {
	
  // mixer_init(dev, devmix);
	
  dev->data_pcm = new NaclAudio();
	
  dev->id   = AUDIO_PCM_NACL;
  dev->fd   = -1;
  dev->open = audio_open;
  dev->close = audio_close;
  dev->write = audio_write;
  dev->writable = audio_writable;
  dev->mix_set = NULL;
  dev->mix_get = NULL;
  dev->exit    = nacl_audio_exit;
	
  return OK;
}
