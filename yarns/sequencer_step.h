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
// SequencerStep

#ifndef YARNS_SEQUENCER_STEP_H_
#define YARNS_SEQUENCER_STEP_H_

namespace yarns {

const int8_t whiteKeyValues[] = {
  0,    0x7f, 1,    0x7f,
  2,    3,    0x7f, 4,
  0x7f, 5,    0x7f, 6,
};
const int8_t blackKeyValues[] = {
  0x7f, 0,    0x7f, 1,
  0x7f, 0x7f, 2,    0x7f,
  3,    0x7f, 4,    0x7f,
};
const int8_t kNumBlackKeys = 5;
const int8_t kNumWhiteKeys = 7;

enum SequencerStepFlags {
  SEQUENCER_STEP_REST = 0x80,
  SEQUENCER_STEP_TIE = 0x81
};

struct SequencerStep {
  // BYTE 0:
  // 0x00 to 0x7f: note
  // 0x80: rest
  // 0x81: tie
  //
  // BYTE 1:
  // 7 bits of velocity + 1 bit for slide flag.
  SequencerStep() { }
  SequencerStep(uint8_t data_0, uint8_t data_1) {
    data[0] = data_0;
    data[1] = data_1;
  }

  uint8_t data[2];

  inline bool has_note() const { return !(data[0] & 0x80); }
  inline bool is_rest() const { return data[0] == SEQUENCER_STEP_REST; }
  inline bool is_tie() const { return data[0] == SEQUENCER_STEP_TIE; }
  inline uint8_t note() const { return data[0] & 0x7f; }

  inline bool is_slid() const { return data[1] & 0x80; }
  inline uint8_t velocity() const { return data[1] & 0x7f; }

  inline bool is_white() const { return whiteKeyValues[note() % 12] != 0x7f; }
  inline int8_t octaves_above_middle_c() const { return ((int8_t) (note() / 12)) - (60 / 12); }
  inline int8_t white_key_value() const { return octaves_above_middle_c() * kNumWhiteKeys + whiteKeyValues[note() % 12]; }
  inline int8_t black_key_value() const { return octaves_above_middle_c() * kNumBlackKeys + blackKeyValues[note() % 12]; }
};

}  // namespace yarns

#endif // YARNS_SEQUENCER_STEP_H_
