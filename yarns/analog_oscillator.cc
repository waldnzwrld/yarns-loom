// Copyright 2012 Emilie Gillet.
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
// Oscillator - analog style waveforms.

#include "yarns/analog_oscillator.h"

#include "stmlib/utils/dsp.h"

#include "braids/resources.h"
#include "braids/parameter_interpolation.h"

namespace braids {

using namespace stmlib;

static const size_t kNumZones = 15;

static const uint16_t kHighestNote = 128 * 128;
static const uint16_t kPitchTableStart = 128 * 128;
static const uint16_t kOctave = 12 * 128;

uint32_t AnalogOscillator::ComputePhaseIncrement(int16_t midi_pitch) {
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

void AnalogOscillator::Render(
    int16_t* buffer,
    size_t size) {
  RenderFn fn = fn_table_[shape_];
  
  if (shape_ != previous_shape_) {
    Init();
    previous_shape_ = shape_;
  }
  
  phase_increment_ = ComputePhaseIncrement(pitch_);
  
  if (pitch_ > kHighestNote) {
    pitch_ = kHighestNote;
  } else if (pitch_ < 0) {
    pitch_ = 0;
  }
  
  (this->*fn)(buffer, size);
}

void AnalogOscillator::RenderCSaw(
    int16_t* buffer,
    size_t size) {
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  int32_t next_sample = next_sample_;
  while (size--) {
    bool self_reset = false;
    INTERPOLATE_PHASE_INCREMENT
    uint32_t pw = static_cast<uint32_t>(parameter_) * 49152;
    if (pw < 8 * phase_increment) {
      pw = 8 * phase_increment;
    }
    
    int32_t this_sample = next_sample;
    next_sample = 0;

    phase_ += phase_increment;
    if (phase_ < phase_increment) {
      self_reset = true;
    }
    
    while (true) {
      if (!high_) {
        if (phase_ < pw) {
          break;
        }
        uint32_t t = (phase_ - pw) / (phase_increment >> 16);
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
        discontinuity_depth_ = -2048 + (aux_parameter_ >> 2);
        uint32_t t = phase_ / (phase_increment >> 16);
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
    *buffer++ = (this_sample - 8192) << 1;
  }
  next_sample_ = next_sample;
  END_INTERPOLATE_PHASE_INCREMENT
}

void AnalogOscillator::RenderSquare(
    int16_t* buffer,
    size_t size) {
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  if (parameter_ > 32000) {
    parameter_ = 32000;
  }
  
  int32_t next_sample = next_sample_;
  while (size--) {
    bool self_reset = false;

    INTERPOLATE_PHASE_INCREMENT
    uint32_t pw = static_cast<uint32_t>(32768 - parameter_) << 16;
    
    int32_t this_sample = next_sample;
    next_sample = 0;
    
    phase_ += phase_increment;
    if (phase_ < phase_increment) {
      self_reset = true;
    }
    
    while (true) {
      if (!high_) {
        if (phase_ < pw) {
          break;
        }
        uint32_t t = (phase_ - pw) / (phase_increment >> 16);
        this_sample += ThisBlepSample(t);
        next_sample += NextBlepSample(t);
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        uint32_t t = phase_ / (phase_increment >> 16);
        this_sample -= ThisBlepSample(t);
        next_sample -= NextBlepSample(t);
        high_ = false;
      }
    }
    
    next_sample += phase_ < pw ? 0 : 32767;
    *buffer++ = (this_sample - 16384) << 1;
  }
  next_sample_ = next_sample;
  END_INTERPOLATE_PHASE_INCREMENT
}

void AnalogOscillator::RenderSaw(
    int16_t* buffer,
    size_t size) {
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  int32_t next_sample = next_sample_;
  while (size--) {
    bool self_reset = false;
    
    INTERPOLATE_PHASE_INCREMENT
    int32_t this_sample = next_sample;
    next_sample = 0;

    phase_ += phase_increment;
    if (phase_ < phase_increment) {
      self_reset = true;
    }

    if (self_reset) {
      uint32_t t = phase_ / (phase_increment >> 16);
      this_sample -= ThisBlepSample(t);
      next_sample -= NextBlepSample(t);
    }
    
    next_sample += phase_ >> 17;
    *buffer++ = (this_sample - 16384) << 1;
  }
  next_sample_ = next_sample;
  END_INTERPOLATE_PHASE_INCREMENT
}

void AnalogOscillator::RenderVariableSaw(
    int16_t* buffer,
    size_t size) {
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  int32_t next_sample = next_sample_;
  if (parameter_ < 1024) {
    parameter_ = 1024;
  }
  while (size--) {
    bool self_reset = false;

    INTERPOLATE_PHASE_INCREMENT
    uint32_t pw = static_cast<uint32_t>(parameter_) << 16;

    int32_t this_sample = next_sample;
    next_sample = 0;

    phase_ += phase_increment;
    if (phase_ < phase_increment) {
      self_reset = true;
    }

    while (true) {
      if (!high_) {
        if (phase_ < pw) {
          break;
        }
        uint32_t t = (phase_ - pw) / (phase_increment >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = true;
      }
      if (high_) {
        if (!self_reset) {
          break;
        }
        self_reset = false;
        uint32_t t = phase_ / (phase_increment >> 16);
        this_sample -= ThisBlepSample(t) >> 1;
        next_sample -= NextBlepSample(t) >> 1;
        high_ = false;
      }
    }
    
    next_sample += phase_ >> 18;
    next_sample += (phase_ - pw) >> 18;
    *buffer++ = (this_sample - 16384) << 1;
  }
  next_sample_ = next_sample;
  END_INTERPOLATE_PHASE_INCREMENT
}

void AnalogOscillator::RenderTriangle(
    int16_t* buffer,
    size_t size) {
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  uint32_t phase = phase_;
  while (size--) {
    INTERPOLATE_PHASE_INCREMENT
    
    int16_t triangle;
    uint16_t phase_16;
    
    phase += phase_increment >> 1;
    phase_16 = phase >> 16;
    triangle = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    triangle += 32768;
    *buffer = triangle >> 1;
    
    phase += phase_increment >> 1;
    phase_16 = phase >> 16;
    triangle = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    triangle += 32768;
    *buffer++ += triangle >> 1;
  }
  phase_ = phase;
  END_INTERPOLATE_PHASE_INCREMENT
}

void AnalogOscillator::RenderSine(
    int16_t* buffer,
    size_t size) {
  uint32_t phase = phase_;
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  while (size--) {
    INTERPOLATE_PHASE_INCREMENT
    phase += phase_increment;
    *buffer++ = Interpolate824(wav_sine, phase);
  }
  END_INTERPOLATE_PHASE_INCREMENT
  phase_ = phase;
}

void AnalogOscillator::RenderTriangleFold(
    int16_t* buffer,
    size_t size) {
  uint32_t phase = phase_;
  
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  BEGIN_INTERPOLATE_PARAMETER
  
  while (size--) {
    INTERPOLATE_PARAMETER
    INTERPOLATE_PHASE_INCREMENT
    
    uint16_t phase_16;
    int16_t triangle;
    int16_t gain = 2048 + (parameter * 30720 >> 15);
    
    // 2x oversampled WF.
    phase += phase_increment >> 1;
    phase_16 = phase >> 16;
    triangle = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    triangle += 32768;
    triangle = triangle * gain >> 15;
    triangle = Interpolate88(ws_tri_fold, triangle + 32768);
    *buffer = triangle >> 1;
    
    phase += phase_increment >> 1;
    phase_16 = phase >> 16;
    triangle = (phase_16 << 1) ^ (phase_16 & 0x8000 ? 0xffff : 0x0000);
    triangle += 32768;
    triangle = triangle * gain >> 15;
    triangle = Interpolate88(ws_tri_fold, triangle + 32768);
    *buffer++ += triangle >> 1;
  }
  
  END_INTERPOLATE_PARAMETER
  END_INTERPOLATE_PHASE_INCREMENT
    
  phase_ = phase;
}

void AnalogOscillator::RenderSineFold(
    int16_t* buffer,
    size_t size) {
  uint32_t phase = phase_;
  
  BEGIN_INTERPOLATE_PHASE_INCREMENT
  BEGIN_INTERPOLATE_PARAMETER
  
  while (size--) {
    INTERPOLATE_PARAMETER
    INTERPOLATE_PHASE_INCREMENT
    
    int16_t sine;
    int16_t gain = 2048 + (parameter * 30720 >> 15);

    
    // 2x oversampled WF.
    phase += phase_increment >> 1;
    sine = Interpolate824(wav_sine, phase);
    sine = sine * gain >> 15;
    sine = Interpolate88(ws_sine_fold, sine + 32768);
    *buffer = sine >> 1;
    
    phase += phase_increment >> 1;
    sine = Interpolate824(wav_sine, phase);
    sine = sine * gain >> 15;
    sine = Interpolate88(ws_sine_fold, sine + 32768);
    *buffer++ += sine >> 1;
  }
  
  END_INTERPOLATE_PARAMETER
  END_INTERPOLATE_PHASE_INCREMENT
  
  phase_ = phase;
}

void AnalogOscillator::RenderBuzz(
    int16_t* buffer,
    size_t size) {
  int32_t shifted_pitch = pitch_ + ((32767 - parameter_) >> 1);
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
  while (size--) {
    phase_ += phase_increment_;
    *buffer++ = Crossfade(wave_1, wave_2, phase_, crossfade);
  }
}

/* static */
AnalogOscillator::RenderFn AnalogOscillator::fn_table_[] = {
  &AnalogOscillator::RenderSaw,
  &AnalogOscillator::RenderVariableSaw,
  &AnalogOscillator::RenderCSaw,
  &AnalogOscillator::RenderSquare,
  &AnalogOscillator::RenderTriangle,
  &AnalogOscillator::RenderSine,
  &AnalogOscillator::RenderTriangleFold,
  &AnalogOscillator::RenderSineFold,
  &AnalogOscillator::RenderBuzz,
};

}  // namespace braids
