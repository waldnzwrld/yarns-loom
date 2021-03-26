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

#define PHASE_ACCELERATION(target, current) \
  current < target ? (target - current) / 64 : ~((current - target) / 64)

// 2kHz refresh / 48kHz audio = 1/24 Since samples are generated in blocks of
// 64, theoretically a Refresh should happen 2-3 times in that period, making
// this interpolated increment stale.  But that requires the envelope to render
// several samples in advance
#define INTERPOLATE(target, current) (target - current) / 24

#define INIT \
  size_t size = kAudioBlockSize; \
  int16_t timbre_increment = INTERPOLATE(timbre_target_, timbre_current_); \
  int32_t gain_increment = INTERPOLATE(gain_target_, gain_current_);

// int16_t parameter_delta = timbre_target_ - timbre_current_;
// int8_t parameter_mod = stmlib::modulo(parameter_delta, kAudioBlockSize);

#define FOLD_GAIN(p) (2048 + (p * 30720 >> 15))

#define INIT_FOLD \
  INIT; \
  int16_t fold_gain = FOLD_GAIN(timbre_current_); \
  int16_t fold_gain_increment = (FOLD_GAIN(timbre_target_) - fold_gain) / 24;

#define ITERATE \
  timbre_current_ += timbre_increment; \
  gain_current_ += gain_increment;

#define ITERATE_FOLD \
  ITERATE; \
  fold_gain += fold_gain_increment;

namespace yarns {

using namespace stmlib;

static const size_t kNumZones = 15;

static const uint16_t kHighestNote = 128 * 128;
static const uint16_t kPitchTableStart = 116 * 128;
static const uint16_t kOctave = 12 * 128;
static const uint16_t kFifth = 7 * 128;

void Oscillator::Refresh(int16_t pitch, int16_t timbre, uint16_t gain) {
    pitch_ = pitch;
    gain_target_ = (scale_ * gain) >> 16;

    int32_t strength = 32767;
    switch (shape_) {
      case OSC_SHAPE_SQUARE:
        CONSTRAIN(timbre, 0, 30000);
        break;
      case OSC_SHAPE_SINE_FOLD:
        strength -= 6 * (pitch_ - (92 << 7));
        CONSTRAIN(strength, 0, 32767);
        timbre = timbre * strength >> 15;
        break;
      /*
      case OSC_SHAPE_TRIANGLE_FOLD:
        strength -= 7 * (pitch_ - (80 << 7));
        CONSTRAIN(strength, 0, 32767);
        timbre = timbre * strength >> 15;
        break;
      */
      default:
        break;
    }
    timbre_target_ = timbre;
  }

uint32_t Oscillator::ComputePhaseIncrement(int16_t midi_pitch) const {
  if (midi_pitch >= kHighestNote) {
    midi_pitch = kHighestNote - 1;
  }
  
  int32_t ref_pitch = midi_pitch;
  ref_pitch -= kPitchTableStart;
  
  size_t num_shifts = 0;
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
  RenderFn fn = fn_table_[shape_];
  
  if (shape_ != previous_shape_) {
    OnShapeChange();
    previous_shape_ = shape_;
  }

  if (samples_.writable() < kAudioBlockSize) return;
  phase_increment_ = ComputePhaseIncrement(pitch_);
  
  if (pitch_ > kHighestNote) {
    pitch_ = kHighestNote;
  } else if (pitch_ < 0) {
    pitch_ = 0;
  }
  
  (this->*fn)();

  // Finish interpolation
  timbre_current_ = timbre_target_;
  gain_current_ = gain_target_;
}

void Oscillator::RenderCSaw() {
  INIT;
  uint32_t pw = static_cast<uint32_t>(timbre_current_) * 49152;
  int32_t pw_increment = static_cast<uint32_t>(timbre_increment) * 49152;
  int32_t next_sample = next_sample_;
  while (size--) {
    ITERATE;
    pw += pw_increment;

    bool self_reset = false;
    if (pw < (phase_increment_ << 3)) {
      pw = phase_increment_ << 3;
    }
    
    int32_t this_sample = next_sample;
    next_sample = 0;

    phase_ += phase_increment_;
    if (phase_ < phase_increment_) {
      self_reset = true;
    }
    
    int16_t shift = -(timbre_current_ - 32767) >> 4;
    while (true) {
      if (!high_) {
        if (phase_ < pw) {
          break;
        }
        uint32_t t = (phase_ - pw) / (phase_increment_ >> 16);
        int16_t before = discontinuity_depth_;
        int16_t after = phase_ >> 18;
        int16_t discontinuity = after - before;
        this_sample += discontinuity * ThisBlepSample(t) >> 15;
        next_sample += discontinuity * NextBlepSample(t) >> 15;
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        discontinuity_depth_ = -2048 + (timbre_current_ >> 2);
        uint32_t t = phase_ / (phase_increment_ >> 16);
        int16_t before = 16383;
        int16_t after = discontinuity_depth_;
        int16_t discontinuity = after - before;
        this_sample += discontinuity * ThisBlepSample(t) >> 15;
        next_sample += discontinuity * NextBlepSample(t) >> 15;
        high_ = false;
      }
    }

    next_sample += phase_ < pw
        ? discontinuity_depth_
        : phase_ >> 18;
    WriteSample(((((this_sample + shift) * 13) >> 3) - 8192) << 1);
  }
  next_sample_ = next_sample;
}

void Oscillator::RenderSquare() {
  INIT;
  int32_t next_sample = next_sample_;
  while (size--) {
    ITERATE;
    bool self_reset = false;

    uint32_t pw = static_cast<uint32_t>(32768 - timbre_current_) << 16;
    
    int32_t this_sample = next_sample;
    next_sample = 0;
    
    phase_ += phase_increment_;
    if (phase_ < phase_increment_) {
      self_reset = true;
    }
    
    while (true) {
      if (!high_) {
        if (phase_ < pw) {
          break;
        }
        uint32_t t = (phase_ - pw) / (phase_increment_ >> 16);
        this_sample += ThisBlepSample(t);
        next_sample += NextBlepSample(t);
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        uint32_t t = phase_ / (phase_increment_ >> 16);
        this_sample -= ThisBlepSample(t);
        next_sample -= NextBlepSample(t);
        high_ = false;
      }
    }
    
    next_sample += phase_ < pw ? 0 : 32767;
    WriteSample((this_sample - 16384) << 1);
  }
  next_sample_ = next_sample;
}

void Oscillator::RenderVariableSaw() {
  INIT;
  int32_t next_sample = next_sample_;
  while (size--) {
    ITERATE;
    bool self_reset = false;

    uint32_t pw = static_cast<uint32_t>(timbre_current_) << 16;

    int32_t this_sample = next_sample;
    next_sample = 0;

    phase_ += phase_increment_;
    if (phase_ < phase_increment_) {
      self_reset = true;
    }

    while (true) {
      if (!high_) {
        if (phase_ < pw) {
          break;
        }
        uint32_t t = (phase_ - pw) / (phase_increment_ >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        uint32_t t = phase_ / (phase_increment_ >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = false;
      }
    }
    
    next_sample += phase_ >> 18;
    next_sample += (phase_ - pw) >> 18;
    WriteSample((this_sample - 16384) << 1);
  }
  next_sample_ = next_sample;
}

void Oscillator::RenderTriangleFold() {
  uint32_t phase = phase_;
  INIT_FOLD;
  while (size--) {
    ITERATE_FOLD;
    uint16_t phase_16;
    int16_t triangle;
    int16_t sample;
    
    phase += phase_increment_;
    phase_16 = phase >> 16;
    triangle = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    triangle += 32768;
    triangle = triangle * fold_gain >> 15;
    triangle = Interpolate88(ws_tri_fold, triangle + 32768);
    sample = triangle;

    WriteSample(sample);
  }
    
  phase_ = phase;
}

void Oscillator::RenderSineFold() {
  uint32_t phase = phase_;
  INIT_FOLD;
  while (size--) {
    ITERATE_FOLD;
    int16_t sine;
    int16_t sample;
    
    phase += phase_increment_;
    sine = Interpolate824(wav_sine, phase);
    sine = sine * fold_gain >> 15;
    sine = Interpolate88(ws_sine_fold, sine + 32768);
    sample = sine;

    WriteSample(sample);
  }

  phase_ = phase;
}

void Oscillator::RenderFM() {
  uint32_t modulator_phase = modulator_phase_;
  uint32_t modulator_phase_increment = ComputePhaseIncrement(
  //    (12 << 7) + pitch_ + ((aux_parameter_ - 16384) >> 1)) >> 1;
    pitch_ + kOctave + kFifth);

  INIT;
  while (size--) {
    ITERATE;
    phase_ += phase_increment_;
    modulator_phase += modulator_phase_increment;

    uint32_t pm = (
        Interpolate824(wav_sine, modulator_phase) * timbre_current_) << 2;
    WriteSample(Interpolate824(wav_sine, phase_ + pm));
  }
  modulator_phase_ = modulator_phase;
}

void Oscillator::RenderSineSync() {
  INIT;
  uint32_t slave_phase = modulator_phase_;
  uint32_t slave_phase_increment = modulator_phase_increment_;
  uint32_t slave_phase_increment_increment = PHASE_ACCELERATION(
    ComputePhaseIncrement(
      pitch_ + (timbre_current_ >> 4)
    ), slave_phase_increment
  );
  int32_t next_sample = next_sample_;
  while (size--) {
    ITERATE;
    slave_phase_increment += slave_phase_increment_increment;
    int32_t this_sample = next_sample;
    next_sample = 0;
    phase_ += phase_increment_;
    if (phase_ < phase_increment_) {
      uint8_t master_sync_time = phase_ / (phase_increment_ >> 7);
      uint32_t master_reset_time = static_cast<uint32_t>(master_sync_time) << 9;
      uint32_t slave_phase_at_reset = slave_phase + \
        (65535 - master_reset_time) * (slave_phase_increment >> 16);
      int32_t before = Interpolate824(wav_sine, slave_phase_at_reset);
      int32_t after = wav_sine[0];
      int32_t discontinuity = after - before;
      this_sample += discontinuity * ThisBlepSample(master_reset_time) >> 15;
      next_sample += discontinuity * NextBlepSample(master_reset_time) >> 15;
      slave_phase = master_reset_time * (slave_phase_increment >> 16);
    } else {
      slave_phase += slave_phase_increment;
    }
    next_sample += Interpolate824(wav_sine, slave_phase);
    WriteSample(next_sample);
  }
  modulator_phase_ = slave_phase;
  modulator_phase_increment_ = slave_phase_increment;
}

const uint32_t kPhaseReset[] = {
  0,
  0x80000000,
  0x40000000,
  0x80000000
};

void Oscillator::RenderDigitalFilter() {
  int16_t shifted_pitch = pitch_ + ((timbre_target_ - 2048) >> 2);
  if (shifted_pitch > 16383) {
    shifted_pitch = 16383;
  }
  uint32_t modulator_phase = modulator_phase_;
  
  uint8_t filter_type = shape_ - OSC_SHAPE_CZ_LP;
  INIT;
  
  uint32_t modulator_phase_increment = modulator_phase_increment_;
  uint32_t target_increment = ComputePhaseIncrement(shifted_pitch);
  uint32_t modulator_phase_increment_increment = PHASE_ACCELERATION(
    target_increment, modulator_phase_increment
  );
    
  while (size--) {
    ITERATE;
    phase_ += phase_increment_;
    modulator_phase_increment += modulator_phase_increment_increment;
    modulator_phase += modulator_phase_increment;
    
    if (phase_ < phase_increment_) {
      modulator_phase = kPhaseReset[filter_type];
    }
    
    int32_t carrier = Interpolate824(wav_sine, modulator_phase);
    
    uint16_t saw = ~(phase_ >> 16);
    // uint16_t triangle = (phase_ >> 15) ^ (phase_ & 0x80000000 ? 0xffff : 0x0000);
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
  }
  modulator_phase_ = modulator_phase;
  modulator_phase_increment_ = modulator_phase_increment;
}

void Oscillator::RenderBuzz() {
  INIT;
  while (size--) {
    ITERATE;
    int32_t shifted_pitch = pitch_ + ((32767 - timbre_current_) >> 1);
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
    phase_ += phase_increment_;
    WriteSample(Crossfade(wave_1, wave_2, phase_, crossfade));
  }
}

void Oscillator::RenderNoise() {
  INIT;
  while (size--) {
    ITERATE;
    WriteSample(Random::GetSample());
  }
}

/* static */
Oscillator::RenderFn Oscillator::fn_table_[] = {
  &Oscillator::RenderVariableSaw,
  &Oscillator::RenderCSaw,
  &Oscillator::RenderSquare,
  &Oscillator::RenderSineSync,
  &Oscillator::RenderFM,
  &Oscillator::RenderDigitalFilter,
  &Oscillator::RenderDigitalFilter,
  &Oscillator::RenderSineFold,
};

}  // namespace yarns
