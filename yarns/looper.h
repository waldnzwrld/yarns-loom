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
// Looper.

#ifndef YARNS_LOOPER_H_
#define YARNS_LOOPER_H_

#include "stmlib/stmlib.h"

#include "yarns/sequencer_step.h"

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

  uint8_t clock_division;

  Recorder() { }
  ~Recorder() { }
  void Init() { }

  void RemoveAll();
  bool IsEmpty();
  void ResetHead();

  void RemoveOldestNote();
  void RemoveNewestNote();

  uint8_t PeekNextOnIndex();
  uint8_t PeekNextOffIndex();
  SequencerStep TryAdvanceOn();
  SequencerStep TryAdvanceOff();
  uint8_t RecordNoteOn(uint16_t pos, SequencerStep step);
  uint8_t RecordNoteOff(uint16_t pos, SequencerStep step);

 private:

  bool Passed(uint16_t target, uint16_t before, uint16_t after);
  void InsertOn(uint8_t index, uint16_t pos);
  void InsertOff(uint8_t index, uint16_t pos);

  Note notes_[kMaxNotes];
  Link head_link_;
  uint8_t oldest_index;
  uint8_t newest_index;

  DISALLOW_COPY_AND_ASSIGN(Recorder);
};

} // namespace looper
}  // namespace yarns

#endif // YARNS_LOOPER_H_
