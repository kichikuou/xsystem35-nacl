/*
 * audio_nacl.cc  NaCl audio lowlevel acess
 *
 * Copyright (C) 2014-2016 <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/
#include "config.h"

extern "C" {
#include "audio.h"
#include "portab.h"
}
#undef min
#undef max

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ppapi/cpp/audio.h>
#include <ppapi/cpp/instance.h>
#include <ppapi_simple/ps.h>

class NaclAudio {
public:
  NaclAudio() : ppaudio_(NULL) {}
  ~NaclAudio() { close(NULL); }
  int open(audiodevice_t* audio, chanfmt_t fmt);
  int close(audiodevice_t* audio);
  int write(audiodevice_t* audio, unsigned char* buf, int cnt);
  bool writable() const { return !full_; }
  bool playing() const { return playing_; }
  void callback(void* samples, uint32_t buffer_size);

private:
  uint32_t sample_frame_count_;
  pp::Audio* ppaudio_;
  void* buf_;
  pthread_cond_t *cond_;
  bool full_;
  bool playing_;
  int zero_count_;
};

void AudioCallback(void* samples, uint32_t buffer_size, void* data) {
  static_cast<NaclAudio*>(data)->callback(samples, buffer_size);
}

int NaclAudio::open(audiodevice_t* audio, chanfmt_t fmt) {
  if (ppaudio_)
    close(audio);

  pp::Instance instance(PSGetInstanceId());

  PP_AudioSampleRate sample_rate = pp::AudioConfig::RecommendSampleRate(&instance);
  if (sample_rate != fmt.rate)
    return NG;

  sample_frame_count_ = pp::AudioConfig::RecommendSampleFrameCount(&instance, sample_rate, 4096);
  if (!sample_frame_count_)
    return NG;

  ppaudio_ = new pp::Audio(&instance,
                           pp::AudioConfig(&instance, sample_rate, sample_frame_count_),
                           AudioCallback,
                           this);
  audio->buf.len = sample_frame_count_ * 4;  // 16 bits, stereo
  buf_ = malloc(audio->buf.len);
  full_ = false;
  playing_ = false;
  zero_count_ = 0;
  cond_ = audio->pcm_cond;
  return OK;
}

int NaclAudio::close(audiodevice_t* audio) {
  if (ppaudio_) {
    delete ppaudio_;
    ppaudio_ = NULL;
    free(buf_);
  }
  return OK;
}

int NaclAudio::write(audiodevice_t* audio, unsigned char* buf, int cnt) {
  if (!ppaudio_ || full_)
    return NG;

  if (!cnt) {
    if (!playing_)
      return OK;

    if (++zero_count_ == 3) {
      ppaudio_->StopPlayback();
      playing_ = false;
    } else {
      memset(buf_, 0, audio->buf.len);
      full_ = true;
    }
  } else {
    zero_count_ = 0;
    assert(cnt == audio->buf.len);
    memcpy(buf_, buf, cnt);
    full_ = true;
    if (!playing_) {
      ppaudio_->StartPlayback();
      playing_ = true;
    }
  }

  return OK;
}

void NaclAudio::callback(void* samples, uint32_t buffer_size) {
  assert(buffer_size >= sample_frame_count_ * 4);
  memcpy(samples, buf_, sample_frame_count_ * 4);
  full_ = false;
  pthread_cond_signal(cond_);
}

static int audio_open(audiodevice_t* audio, chanfmt_t fmt) {
  return static_cast<NaclAudio*>(audio->data_pcm)->open(audio, fmt);
}

static int audio_close(audiodevice_t* audio) {
  return static_cast<NaclAudio*>(audio->data_pcm)->close(audio);
}

static int audio_write(audiodevice_t* audio, unsigned char* buf, int cnt) {
  return static_cast<NaclAudio*>(audio->data_pcm)->write(audio, buf, cnt);
}

static boolean audio_writable(audiodevice_t* audio) {
  return static_cast<NaclAudio*>(audio->data_pcm)->writable() ? TRUE : FALSE;
}

static boolean audio_playing(audiodevice_t* audio) {
  return static_cast<NaclAudio*>(audio->data_pcm)->playing() ? TRUE : FALSE;
}

static void mixer_set_level(audiodevice_t* dev, int ch, int level) {
  fprintf(stderr, "mixer_set_level: not implemented\n");
}

static int mixer_get_level(audiodevice_t* dev, int ch) {
  fprintf(stderr, "mixer_get_level: not implemented\n");
  return 100;
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
  dev->playing = audio_playing;
  dev->mix_set = mixer_set_level;
  dev->mix_get = mixer_get_level;
  dev->exit    = nacl_audio_exit;
	
  return OK;
}
