// Copyright 2019 Chris Rogers.
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
// Voice.

#ifndef YARNS_LOOPER_H_
#define YARNS_LOOPER_H_

#include "yarns/sequencer_step.h"
//#include "yarns/part.h"

namespace yarns {

namespace looper {

const uint8_t kMaxNotes = 64;

struct Link {
  Link() { }
  uint8_t on_index;
  uint8_t off_index;
};

struct Note {
  Note() { }
  Link next_link;
  SequencerStep step;
  uint16_t on_pos;
  uint16_t off_pos;
};

class Recorder {
 public:

  Recorder() { }
  ~Recorder() { }
  void Init();

  void SetNotesCount(uint8_t n) {
    notes_count_ = n;
  }

 private:

  Note notes_[kMaxNotes];
  uint8_t notes_count_;
  uint8_t current_pos_;
  Link head_link_;
  uint8_t oldest_index;
  uint8_t newest_index;
  uint8_t note_index_for_pressed_key_index_[12];

  DISALLOW_COPY_AND_ASSIGN(Recorder);
};

} // namespace looper
}  // namespace yarns

#endif // YARNS_LOOPER_H_
