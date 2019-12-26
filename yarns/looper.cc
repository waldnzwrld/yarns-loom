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

#include "yarns/looper.h"

namespace yarns {

namespace looper {

void Recorder::RemoveAll() {
  std::fill(
    &notes_[0],
    &notes_[kMaxNotes],
    Note()
  );
  head_link_.on_index = kNullIndex;
  head_link_.off_index = kNullIndex;
  oldest_index_ = 0;
  newest_index_ = 0;
}

void Recorder::ResetHead() {
  uint8_t next_index;

  while (true) {
    next_index = PeekNextOnIndex();
    if (
      next_index == kNullIndex ||
      notes_[head_link_.on_index].on_pos >= notes_[next_index].on_pos
    ) {
      break;
    }
    head_link_.on_index = next_index;
  }

  while (true) {
    next_index = PeekNextOffIndex();
    if (
      next_index == kNullIndex ||
      notes_[head_link_.off_index].off_pos >= notes_[next_index].off_pos
    ) {
      break;
    }
    head_link_.off_index = next_index;
  }
}

uint8_t Recorder::PeekNextOnIndex() {
  if (head_link_.on_index == kNullIndex) {
    return kNullIndex;
  }
  return notes_[head_link_.on_index].next_link.on_index;
}

uint8_t Recorder::PeekNextOffIndex() {
  if (head_link_.off_index == kNullIndex) {
    return kNullIndex;
  }
  return notes_[head_link_.off_index].next_link.off_index;
}

SequencerStep Recorder::TryAdvanceOn(uint16_t old_pos, uint16_t new_pos) {
  uint8_t next_index = PeekNextOnIndex();
  const Note& next_note = notes_[next_index];
  if (!Passed(next_note.on_pos, old_pos, new_pos)) {
    return SequencerStep(SEQUENCER_STEP_REST, 0);
  }
  head_link_.on_index = next_index;

  // If this note doesn't yet have an off_pos, set one and leave it on
  if (next_note.next_link.off_index == kNullIndex) {
    // TODO should insert_off further back to let the gate go low?
    InsertOff(next_index, next_note.on_pos);
    return SequencerStep(SEQUENCER_STEP_REST, 0);
  }

  return next_note.step;
}

SequencerStep Recorder::TryAdvanceOff(uint16_t old_pos, uint16_t new_pos) {
  uint8_t next_index = PeekNextOffIndex();
  const Note& next_note = notes_[next_index];
  if (!Passed(next_note.off_pos, old_pos, new_pos)) {
    return SequencerStep(SEQUENCER_STEP_REST, 0);
  }
  head_link_.off_index = next_index;

  return next_note.step;
}

bool Recorder::Passed(uint16_t target, uint16_t before, uint16_t after) {
  if (before < after) {
    return (target > before and target <= after);
  } else {
    return (target > before or target <= after);
  }
}

void Recorder::InsertOn(uint8_t index, uint16_t pos) {
  Note& note = notes_[index];
  note.on_pos = pos;
  if (head_link_.on_index == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    note.next_link.on_index = index;
  } else {
    Note& head_note = notes_[head_link_.on_index];
    note.next_link.on_index = head_note.next_link.on_index;
    head_note.next_link.on_index = index;
  }
  head_link_.on_index = index;
}

void Recorder::InsertOff(uint8_t index, uint16_t pos) {
  Note& note = notes_[index];
  note.off_pos = pos;
  if (head_link_.off_index == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    note.next_link.off_index = index;
  } else {
    Note& head_note = notes_[head_link_.off_index];
    note.next_link.off_index = head_note.next_link.off_index;
    head_note.next_link.off_index = index;
  }
  head_link_.off_index = index;
}

} // namespace looper
}  // namespace yarns
