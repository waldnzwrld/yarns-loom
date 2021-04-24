// Copyright 2012 Emilie Gillet.
// Copyright 2021 Chris Rogers.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Oscillator.

#include "yarns/oscillator.h"

#include "stmlib/utils/dsp.h"
#include "stmlib/utils/random.h"

#include "yarns/resources.h"

#define RENDER_LOOP(body) \
  size_t size = kAudioBlockSize; \
  timbre_.ComputeSlope(); gain_.ComputeSlope(); \
  int32_t next_sample = next_sample_; \
  uint32_t phase = phase_; \
  uint32_t phase_increment = phase_increment_; \
  uint32_t modulator_phase = modulator_phase_; \
  uint32_t modulator_phase_increment = modulator_phase_increment_; \
  while (size--) { \
    phase += phase_increment; \
    modulator_phase += modulator_phase_increment; \
    timbre_.Tick(); gain_.Tick(); \
    body \
  } \
  next_sample_ = next_sample; \
  phase_ = phase; \
  phase_increment_ = phase_increment; \
  modulator_phase_ = modulator_phase; \
  modulator_phase_increment_ = modulator_phase_increment;

namespace yarns {

using namespace stmlib;

static const size_t kNumZones = 15;

static const uint16_t kHighestNote = 128 * 128;
static const uint16_t kPitchTableStart = 116 * 128;
static const uint16_t kOctave = 12 * 128;

void Oscillator::Refresh(int16_t pitch, int16_t timbre, uint16_t gain) {
    pitch_ = pitch;
    if (shape_ >= OSC_SHAPE_FM) {
      pitch_ += lut_fm_carrier_corrections[shape_ - OSC_SHAPE_FM];
    }
    gain_.SetTarget((scale_ * gain) >> 17);

    int32_t strength = 32767;
    if (shape_ == OSC_SHAPE_SINE_FOLD || shape_ >= OSC_SHAPE_FM) {
      strength -= 6 * (pitch_ - (92 << 7));
      CONSTRAIN(strength, 0, 32767);
      timbre = timbre * strength >> 15;
    } else {
      switch (shape_) {
        case OSC_SHAPE_SQUARE:
          CONSTRAIN(timbre, 0, 31767);
          break;
        case OSC_SHAPE_TRIANGLE_FOLD:
          strength -= 7 * (pitch_ - (80 << 7));
          CONSTRAIN(strength, 0, 32767);
          timbre = timbre * strength >> 15;
          break;
        default:
          break;
      }
    }
    timbre_.SetTarget(timbre);
  }

uint32_t Oscillator::ComputePhaseIncrement(int16_t midi_pitch) const {
  int32_t num_shifts = 0;
  while (midi_pitch >= kHighestNote) {
    midi_pitch -= kOctave;
    --num_shifts;
  }
  int32_t ref_pitch = midi_pitch;
  ref_pitch -= kPitchTableStart;
  while (ref_pitch < 0) {
    ref_pitch += kOctave;
    ++num_shifts;
  }
  
  uint32_t a = lut_oscillator_increments[ref_pitch >> 4];
  uint32_t b = lut_oscillator_increments[(ref_pitch >> 4) + 1];
  uint32_t phase_increment = a + \
      (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
  phase_increment >>= num_shifts;
  return phase_increment;
}

void Oscillator::Render() {
  if (audio_buffer_.writable() < kAudioBlockSize) return;
  
  if (pitch_ > kHighestNote) {
    pitch_ = kHighestNote;
  } else if (pitch_ < 0) {
    pitch_ = 0;
  }
  phase_increment_ = ComputePhaseIncrement(pitch_);
  
  uint8_t fn_index = shape_;
  CONSTRAIN(fn_index, 0, OSC_SHAPE_FM);
  RenderFn fn = fn_table_[fn_index];
  (this->*fn)();
}

void Oscillator::RenderSquare() {
  RENDER_LOOP(
    bool self_reset = false;
    uint32_t pw = static_cast<uint32_t>(32768 - timbre_.value()) << 16;
    int32_t this_sample = next_sample;
    next_sample = 0;
    if (phase < phase_increment) {
      self_reset = true;
    }
    while (true) {
      if (!high_) {
        if (phase < pw) {
          break;
        }
        uint32_t t = (phase - pw) / (phase_increment >> 16);
        this_sample += ThisBlepSample(t);
        next_sample += NextBlepSample(t);
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        uint32_t t = phase / (phase_increment >> 16);
        this_sample -= ThisBlepSample(t);
        next_sample -= NextBlepSample(t);
        high_ = false;
      }
    }
    next_sample += phase < pw ? 0 : 32767;
    WriteSample((this_sample - 16384) << 1);
  )
}

void Oscillator::RenderVariableSaw() {
  RENDER_LOOP(
    bool self_reset = false;
    uint32_t pw = static_cast<uint32_t>(timbre_.value()) << 16;
    int32_t this_sample = next_sample;
    next_sample = 0;
    if (phase < phase_increment) {
      self_reset = true;
    }
    while (true) {
      if (!high_) {
        if (phase < pw) {
          break;
        }
        uint32_t t = (phase - pw) / (phase_increment >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        uint32_t t = phase / (phase_increment >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = false;
      }
    }
    next_sample += phase >> 18;
    next_sample += (phase - pw) >> 18;
    WriteSample((this_sample - 16384) << 1);
  )
}

void Oscillator::RenderTriangleFold() {
  RENDER_LOOP(
    uint16_t phase_16 = phase >> 16;
    int16_t sample = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    sample += 32768;
    sample = sample * timbre_.value() >> 15;
    sample = Interpolate88(ws_tri_fold, sample + 32768);
    WriteSample(sample);
  )
}

void Oscillator::RenderSineFold() {
  RENDER_LOOP(
    int16_t sample = Interpolate824(wav_sine, phase);
    sample = sample * timbre_.value() >> 15;
    sample = Interpolate88(ws_sine_fold, sample + 32768);
    WriteSample(sample);
  )
}

void Oscillator::RenderFM() {
  int16_t interval = lut_fm_modulator_intervals[shape_ - OSC_SHAPE_FM];
  modulator_phase_increment_ = ComputePhaseIncrement(pitch_ + interval);
  RENDER_LOOP(
    int16_t modulator = Interpolate824(wav_sine, modulator_phase);
    uint32_t phase_mod = (modulator * timbre_.value()) << 2;
    WriteSample(Interpolate824(wav_sine, phase + phase_mod));
  )
}

void Oscillator::RenderSineSync() {
  modulator_phase_increment_ = ComputePhaseIncrement(
    pitch_ + (timbre_.target() >> 4)
  );
  RENDER_LOOP(
    int32_t this_sample = next_sample;
    next_sample = 0;
    if (phase < phase_increment) {
      modulator_phase -= modulator_phase_increment; // Modulator resets instead
      uint8_t master_sync_time = phase / (phase_increment >> 7);
      uint32_t master_reset_time = static_cast<uint32_t>(master_sync_time) << 9;
      uint32_t modulator_phase_at_reset = modulator_phase + \
        (65535 - master_reset_time) * (modulator_phase_increment >> 16);
      int32_t before = 0;
      int32_t after = 0;
      switch (shape_) {
        case OSC_SHAPE_SINE_SYNC:
          before = Interpolate824(wav_sine, modulator_phase_at_reset);
          after = wav_sine[0];
          break;
        default: break;
      }
      int32_t discontinuity = after - before;
      this_sample += discontinuity * ThisBlepSample(master_reset_time) >> 15;
      next_sample += discontinuity * NextBlepSample(master_reset_time) >> 15;
      modulator_phase = master_reset_time * (modulator_phase_increment >> 16);
    }
    switch (shape_) {
      case OSC_SHAPE_SINE_SYNC:
        next_sample += Interpolate824(wav_sine, modulator_phase);
        break;
      default: break;
    }
    WriteSample(next_sample);
  )
}

const uint32_t kPhaseReset[] = {
  0,
  0x80000000,
  0x40000000,
  0x80000000
};

void Oscillator::RenderDigitalFilter() {
  int16_t shifted_pitch = pitch_ + ((timbre_.target() - 2048) >> 2);
  if (shifted_pitch > 16383) {
    shifted_pitch = 16383;
  }
  uint8_t filter_type = shape_ - OSC_SHAPE_CZ_LP;
  uint32_t modulator_phase_increment_ = ComputePhaseIncrement(shifted_pitch);
  RENDER_LOOP(
    if (phase < phase_increment) {
      modulator_phase = kPhaseReset[filter_type];
    }
    int32_t carrier = Interpolate824(wav_sine, modulator_phase);
    uint16_t saw = ~(phase >> 16);
    // uint16_t triangle = (phase >> 15) ^ (phase & 0x80000000 ? 0xffff : 0x0000);
    uint16_t window = saw; // aux_parameter_ < 16384 ? saw : triangle;
    int16_t saw_tri_signal;
    if (filter_type & 2) {
      saw_tri_signal = (carrier * window) >> 16;
    } else {
      saw_tri_signal = (window * (carrier + 32768) >> 16) - 32768;
    }
    // uint16_t balance = (aux_parameter_ < 16384 ? 
    //                     aux_parameter_ : ~aux_parameter_) << 2;
    WriteSample(saw_tri_signal);
  )
}

void Oscillator::RenderBuzz() {
  RENDER_LOOP(
    int32_t shifted_pitch = pitch_ + ((32767 - timbre_.value()) >> 1);
    uint16_t crossfade = shifted_pitch << 6;
    size_t index = (shifted_pitch >> 10);
    if (index >= kNumZones) {
      index = kNumZones - 1;
    }
    const int16_t* wave_1 = waveform_table[WAV_BANDLIMITED_COMB_0 + index];
    index += 1;
    if (index >= kNumZones) {
      index = kNumZones - 1;
    }
    const int16_t* wave_2 = waveform_table[WAV_BANDLIMITED_COMB_0 + index];
    WriteSample(Crossfade(wave_1, wave_2, phase, crossfade));
  )
}

void Oscillator::RenderFilteredNoise() {
  svf_.cutoff.SetTarget(Interpolate824(lut_svf_cutoff, timbre_.target() << 17) >> 1);
  svf_.damp.SetTarget(Interpolate824(lut_svf_damp, pitch_ << 18) >> 1);
  svf_.cutoff.ComputeSlope();
  svf_.damp.ComputeSlope();
  // int32_t scale = Interpolate824(lut_svf_scale, pitch_ << 18);
  // int32_t gain_correction = cutoff > scale ? scale * 32767 / cutoff : 32767;
  int32_t bp = svf_.bp;
  int32_t lp = svf_.lp;
  int32_t notch, hp, in, result;
  RENDER_LOOP(
    svf_.cutoff.Tick();
    svf_.damp.Tick();
    in = Random::GetSample() >> 1;
    notch = in - (bp * svf_.damp.value() >> 15);
    lp += svf_.cutoff.value() * bp >> 15;
    CONSTRAIN(lp, -16384, 16383)
    hp = notch - lp;
    bp += svf_.cutoff.value() * hp >> 15;
    switch (shape_) {
      case OSC_SHAPE_NOISE_LP: result = lp; break;
      case OSC_SHAPE_NOISE_NOTCH: result = notch; break;
      case OSC_SHAPE_NOISE_BP: result = bp; break;
      case OSC_SHAPE_NOISE_HP: result = hp; break;
      default: result = 0; break;
    }
    CONSTRAIN(result, -16384, 16383)
    // result = result * gain_correction >> 15;
    // result = Interpolate88(ws_moderate_overdrive, result + 32768);
    WriteSample(result << 1);
  )
  svf_.lp = lp;
  svf_.bp = bp;
}

/* static */
Oscillator::RenderFn Oscillator::fn_table_[] = {
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderSquare,
  &Oscillator::RenderVariableSaw,
  &Oscillator::RenderSineSync,
  &Oscillator::RenderDigitalFilter,
  &Oscillator::RenderDigitalFilter,
  &Oscillator::RenderDigitalFilter,
  &Oscillator::RenderDigitalFilter,
  &Oscillator::RenderTriangleFold,
  &Oscillator::RenderSineFold,
  &Oscillator::RenderBuzz,
  &Oscillator::RenderFM,
};

}  // namespace yarns
