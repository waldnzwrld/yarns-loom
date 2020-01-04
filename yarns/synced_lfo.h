// Copyright 2013 Emilie Gillet.
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
// Synced LFO.

#ifndef YARNS_SYNCED_LFO_H_
#define YARNS_SYNCED_LFO_H_

namespace yarns {

class SyncedLFO {
 public:

  uint8_t clock_division_;

  SyncedLFO() { }
  ~SyncedLFO() { }
  void Init() {
    phase_ = 0;
  }

  uint32_t GetPhase() {
    return phase_;
  }

  uint32_t GetPhaseIncrement() {
    return phase_increment_;
  }

  uint32_t Increment(uint32_t increment) {
    phase_ += increment;
    return GetPhase();
  }

  uint32_t Refresh() {
    return Increment(phase_increment_);
  }

  void Tap(uint32_t target_phase) {
    uint32_t target_increment = target_phase - previous_target_phase_;

    int32_t d_error = target_increment - (phase_ - previous_phase_);
    int32_t p_error = target_phase - phase_;
    int32_t error = (d_error + (p_error >> 1)) >> 13;

    if (error < 0 && abs(error) > phase_increment_) {
      // underflow
      phase_increment_ = 0;
    } else if (error > 0 && (UINT32_MAX - error) < phase_increment_) {
      // overflow
      phase_increment_ = UINT32_MAX;
    } else {
      phase_increment_ += error;
    }

    previous_phase_ = phase_;
    previous_target_phase_ = target_phase;
  }

 private:

  uint32_t phase_;
  uint32_t phase_increment_;
  uint32_t previous_target_phase_;
  uint32_t previous_phase_;

  DISALLOW_COPY_AND_ASSIGN(SyncedLFO);
};

} // namespace yarns

#endif // YARNS_SYNCED_LFO_H_
