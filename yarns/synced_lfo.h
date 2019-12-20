// Copyright 2013 Emilie Gillet.
//
// Author: Chris Rogers (teukros@gmail.com)
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
  void Init();

  void Tap(uint32_t current_phase, uint32_t target_phase) {
    uint32_t target_increment = target_phase - previous_target_phase_;

    int32_t d_error = target_increment - (current_phase - previous_phase_);
    int32_t p_error = target_phase - current_phase;
    int32_t error = d_error + (p_error >> 1);

    phase_increment_ += error >> 11;

    previous_phase_ = current_phase;
    previous_target_phase_ = target_phase;
  }

  uint32_t GetPhaseIncrement() {
    return phase_increment_;
  }

 private:

  uint32_t phase_increment_;
  uint32_t previous_target_phase_;
  uint32_t previous_phase_;

  DISALLOW_COPY_AND_ASSIGN(SyncedLFO);
};

} // namespace yarns

#endif // YARNS_SYNCED_LFO_H_
