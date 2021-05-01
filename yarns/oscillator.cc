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
    int32_t this_sample = next_sample; \
    next_sample = 0; \
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

void StateVariableFilter::Init(uint8_t interpolation_slope) {
  cutoff.Init(interpolation_slope);
  damp.Init(interpolation_slope);
}

void StateVariableFilter::RenderInit(uint32_t frequency, uint32_t resonance) {
  cutoff.SetTarget(Interpolate824(lut_svf_cutoff, frequency) >> 1);
  damp.SetTarget(Interpolate824(lut_svf_damp, resonance) >> 1);
  cutoff.ComputeSlope();
  damp.ComputeSlope();
}

void StateVariableFilter::RenderTick(int16_t in) {
  cutoff.Tick();
  damp.Tick();
  notch = in - (bp * damp.value() >> 15);
  lp += cutoff.value() * bp >> 15;
  CONSTRAIN(lp, -16384, 16383)
  hp = notch - lp;
  bp += cutoff.value() * hp >> 15;
}

void Oscillator::Refresh(int16_t pitch, int16_t timbre, uint16_t gain) {
    pitch_ = pitch;
    // if (shape_ >= OSC_SHAPE_FM) {
    //   pitch_ += lut_fm_carrier_corrections[shape_ - OSC_SHAPE_FM];
    // }
    gain_.SetTarget((scale_ * gain) >> 17);

    int32_t strength = 32767;
    if (shape_ == OSC_SHAPE_FOLD_SINE || shape_ >= OSC_SHAPE_FM) {
      strength -= 6 * (pitch_ - (92 << 7));
      CONSTRAIN(strength, 0, 32767);
      timbre = timbre * strength >> 15;
    } else {
      switch (shape_) {
        case OSC_SHAPE_VARIABLE_PULSE:
          CONSTRAIN(timbre, 0, 31767);
          break;
        case OSC_SHAPE_FOLD_TRIANGLE:
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
  int16_t num_shifts = 0;
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
  if (num_shifts > 0) phase_increment >>= num_shifts;
  else if (num_shifts < 0) {
    int16_t shifts_available = __builtin_clz(phase_increment) - 1;
    num_shifts = -num_shifts;
    num_shifts = std::min(shifts_available, num_shifts);
    phase_increment <<= num_shifts;
  }
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

void Oscillator::RenderPulse() {
  svf_.RenderInit(timbre_.target() << 17, 0x80000000);
  modulator_phase_increment_ = phase_increment_;
  RENDER_LOOP(
    bool self_reset = modulator_phase < modulator_phase_increment;
    uint32_t pw = shape_ == OSC_SHAPE_VARIABLE_PULSE
      ? static_cast<uint32_t>(32768 - timbre_.value()) << 16
      : 0x80000000;
    while (true) {
      if (!high_) {
        if (modulator_phase < pw) break;
        uint32_t t = (modulator_phase - pw) / (modulator_phase_increment >> 16);
        this_sample += ThisBlepSample(t);
        next_sample += NextBlepSample(t);
        high_ = true;
      }
      if (high_) {
        if (!self_reset) break;
        self_reset = false;
        uint32_t t = modulator_phase / (modulator_phase_increment >> 16);
        this_sample -= ThisBlepSample(t);
        next_sample -= NextBlepSample(t);
        high_ = false;
      }
    }
    next_sample += modulator_phase < pw ? 0 : 32767;
    this_sample = (this_sample - 16384) << 1;
    svf_.RenderTick(this_sample);
    WriteSample(shape_ == OSC_SHAPE_FILTERED_PULSE ? svf_.lp : this_sample);
  )
}

void Oscillator::RenderSaw() {
  svf_.RenderInit(timbre_.target() << 17, 0x80000000);
  modulator_phase_increment_ = phase_increment_;
  RENDER_LOOP(
    bool self_reset = modulator_phase < modulator_phase_increment;
    uint32_t pw = shape_ == OSC_SHAPE_VARIABLE_SAW
      ? static_cast<uint32_t>(32768 - timbre_.value()) << 16
      : 0x80000000;
    while (true) {
      if (!high_) {
        if (modulator_phase < pw) break;
        uint32_t t = (modulator_phase - pw) / (modulator_phase_increment >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = true;
      }
      if (high_) {
        if (!self_reset) break;
        self_reset = false;
        uint32_t t = modulator_phase / (modulator_phase_increment >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = false;
      }
    }
    next_sample += modulator_phase >> 18;
    next_sample += (modulator_phase - pw) >> 18;
    this_sample = (this_sample - 16384) << 1;
    svf_.RenderTick(this_sample);
    WriteSample(shape_ == OSC_SHAPE_FILTERED_SAW ? svf_.lp : this_sample);
  )
}

void Oscillator::RenderFoldTriangle() {
  RENDER_LOOP(
    uint16_t phase_16 = phase >> 16;
    this_sample = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    this_sample += 32768;
    this_sample = this_sample * timbre_.value() >> 15;
    this_sample = Interpolate88(ws_tri_fold, this_sample + 32768);
    WriteSample(this_sample);
  )
}

void Oscillator::RenderFoldSine() {
  RENDER_LOOP(
    this_sample = Interpolate824(wav_sine, phase);
    this_sample = this_sample * timbre_.value() >> 15;
    this_sample = Interpolate88(ws_sine_fold, this_sample + 32768);
    WriteSample(this_sample);
  )
}

void Oscillator::RenderFM() {
  int16_t interval = lut_fm_modulator_intervals[shape_ - OSC_SHAPE_FM];
  modulator_phase_increment_ = ComputePhaseIncrement(pitch_ + interval);
  RENDER_LOOP(
    int16_t modulator = Interpolate824(wav_sine, modulator_phase);
    uint32_t phase_mod = (modulator * timbre_.value()) << 3;
    this_sample = Interpolate824(wav_sine, phase + phase_mod);
    WriteSample(this_sample);
  )
}

void Oscillator::RenderSineSync() {
  modulator_phase_increment_ = ComputePhaseIncrement(
    pitch_ + (timbre_.target() >> 4)
  );
  RENDER_LOOP(
    if (phase < phase_increment) {
      modulator_phase -= modulator_phase_increment; // Modulator resets instead
      uint8_t master_sync_time = phase / (phase_increment >> 7);
      uint32_t master_reset_time = static_cast<uint32_t>(master_sync_time) << 9;
      uint32_t modulator_phase_at_reset = modulator_phase + \
        (65535 - master_reset_time) * (modulator_phase_increment >> 16);
      int32_t before = 0;
      int32_t after = 0;
      switch (shape_) {
        case OSC_SHAPE_SYNC_SINE:
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
      case OSC_SHAPE_SYNC_SINE:
        next_sample += Interpolate824(wav_sine, modulator_phase);
        break;
      default: break;
    }
    WriteSample(this_sample);
  )
}

const uint32_t kPhaseReset[] = {
  0,
  0x80000000,
  0x40000000,
  0x80000000
};

void Oscillator::RenderPhaseDistortion() {
  int16_t shifted_pitch = pitch_ + ((timbre_.target() - 2048) >> 2);
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
    if (filter_type & 2) {
      this_sample = (carrier * window) >> 16;
    } else {
      this_sample = (window * (carrier + 32768) >> 16) - 32768;
    }
    // uint16_t balance = (aux_parameter_ < 16384 ? 
    //                     aux_parameter_ : ~aux_parameter_) << 2;
    WriteSample(this_sample);
  )
}

void Oscillator::RenderBuzz() {
  RENDER_LOOP(
    int32_t zone_14 = (pitch_ + ((32767 - timbre_.value()) >> 1));
    uint16_t crossfade = zone_14 << 6; // Ignore highest 4 bits
    size_t index = zone_14 >> 10; // Use highest 4 bits
    CONSTRAIN(index, 0, kNumZones - 1);
    const int16_t* wave_1 = waveform_table[WAV_BANDLIMITED_COMB_0 + index];
    index += 1;
    CONSTRAIN(index, 0, kNumZones - 1);
    const int16_t* wave_2 = waveform_table[WAV_BANDLIMITED_COMB_0 + index];
    this_sample = Crossfade(wave_1, wave_2, phase, crossfade);
    WriteSample(this_sample);
  )
}

void Oscillator::RenderFilteredNoise() {
  svf_.RenderInit(timbre_.target() << 17, pitch_ << 18);
  // int32_t scale = Interpolate824(lut_svf_scale, pitch_ << 18);
  // int32_t gain_correction = cutoff > scale ? scale * 32767 / cutoff : 32767;
  RENDER_LOOP(
    svf_.RenderTick(Random::GetSample() >> 1);
    switch (shape_) {
      case OSC_SHAPE_NOISE_LP: this_sample = svf_.lp; break;
      case OSC_SHAPE_NOISE_NOTCH: this_sample = svf_.notch; break;
      case OSC_SHAPE_NOISE_BP: this_sample = svf_.bp; break;
      case OSC_SHAPE_NOISE_HP: this_sample = svf_.hp; break;
      default: break;
    }
    CONSTRAIN(this_sample, -16384, 16383)
    // result = result * gain_correction >> 15;
    // result = Interpolate88(ws_moderate_overdrive, result + 32768);
    WriteSample(this_sample << 1);
  )
}

/* static */
Oscillator::RenderFn Oscillator::fn_table_[] = {
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderFilteredNoise,
  &Oscillator::RenderPulse,
  &Oscillator::RenderSaw,
  &Oscillator::RenderPulse,
  &Oscillator::RenderSaw,
  &Oscillator::RenderSineSync,
  &Oscillator::RenderPhaseDistortion,
  &Oscillator::RenderPhaseDistortion,
  &Oscillator::RenderPhaseDistortion,
  &Oscillator::RenderPhaseDistortion,
  &Oscillator::RenderFoldSine,
  &Oscillator::RenderFoldTriangle,
  &Oscillator::RenderBuzz,
  &Oscillator::RenderFM,
};

}  // namespace yarns
