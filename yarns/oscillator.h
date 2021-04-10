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
// Oscillator - analog style waveforms.

#ifndef YARNS_ANALOG_OSCILLATOR_H_
#define YARNS_ANALOG_OSCILLATOR_H_

#include "stmlib/stmlib.h"
#include "stmlib/utils/ring_buffer.h"

#include <cstring>
#include <cstdio>

namespace yarns {

const size_t kAudioBlockSize = 64;

enum OscillatorShape {
  OSC_SHAPE_BUZZ,
  OSC_SHAPE_FM,
  OSC_SHAPE_SINE_FOLD,
  OSC_SHAPE_TRIANGLE_FOLD,
  OSC_SHAPE_CZ_LP,
  OSC_SHAPE_CZ_PK,
  OSC_SHAPE_SINE_SYNC,
  OSC_SHAPE_SQUARE,
  OSC_SHAPE_CSAW,
  OSC_SHAPE_VARIABLE_SAW,
  OSC_SHAPE_NOISE,

  OSC_SHAPE_LAST
};

class Oscillator {
 public:
  typedef void (Oscillator::*RenderFn)();

  Oscillator() { }
  ~Oscillator() { }

  inline void Init(int32_t scale, int32_t offset) {
    audio_buffer_.Init();
    scale_ = scale;
    offset_ = offset;
    timbre_current_ = timbre_target_ = 0;
    gain_current_ = gain_target_ = 0;
    pitch_ = 60 << 7;
    OnShapeChange();
  }
  
  inline void OnShapeChange() {
    phase_ = 0;
    phase_increment_ = 1;
    high_ = false;
    discontinuity_depth_ = -16383;
    next_sample_ = 0;
  }

  inline void WriteSample(int16_t sample) {
    audio_buffer_.Overwrite(offset_ - ((gain_current_ * sample) >> 16));
  }

  inline uint16_t ReadSample() {
    return audio_buffer_.ImmediateRead();
  }

  void Refresh(int16_t pitch, int16_t timbre, uint16_t gain);
  
  inline void set_shape(OscillatorShape shape) {
    shape_ = shape;
  }
  
  void Render();
  
 private:
  void RenderSquare();
  void RenderVariableSaw();
  void RenderCSaw();
  void RenderTriangleFold();
  void RenderSineFold();
  void RenderFM();
  void RenderSineSync();
  void RenderDigitalFilter();
  void RenderBuzz();
  void RenderNoise();
  
  uint32_t ComputePhaseIncrement(int16_t midi_pitch) const;
  
  inline int32_t ThisBlepSample(uint32_t t) const {
    if (t > 65535) {
      t = 65535;
    }
    return t * t >> 18;
  }
  
  inline int32_t NextBlepSample(uint32_t t) const {
    if (t > 65535) {
      t = 65535;
    }
    t = 65535 - t;
    return -static_cast<int32_t>(t * t >> 18);
  }
   
  uint32_t phase_;
  uint32_t phase_increment_;
  uint32_t previous_phase_increment_;
  bool high_;

  // 15-bit
  int16_t timbre_current_;
  int16_t timbre_target_;

  int32_t gain_current_;
  int32_t gain_target_;

  int16_t discontinuity_depth_;
  int16_t pitch_;
  uint32_t modulator_phase_;
  uint32_t modulator_phase_increment_;
  
  int32_t next_sample_;
  
  OscillatorShape shape_;
  OscillatorShape previous_shape_;

  int32_t scale_;
  int32_t offset_;
  stmlib::RingBuffer<uint16_t, kAudioBlockSize * 2> audio_buffer_;
  
  static RenderFn fn_table_[];
  
  DISALLOW_COPY_AND_ASSIGN(Oscillator);
};

}  // namespace yarns

#endif // YARNS_ANALOG_OSCILLATOR_H_
