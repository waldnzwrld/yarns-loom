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

#include "yarns/part.h"

namespace yarns {

namespace looper {

void Deck::Init(Part* part) {
  part_ = part;
  tape_ = &(part->mutable_sequencer_settings()->looper_tape);
}

void Deck::RemoveAll() {
  std::fill(
    &tape_->notes[0],
    &tape_->notes[kMaxNotes],
    Note()
  );
  tape_->head_link.on_index = kNullIndex;
  tape_->head_link.off_index = kNullIndex;
  tape_->oldest_index = 0;
  tape_->newest_index = 0;
}

void Deck::ResetHead() {
  uint8_t next_index;

  while (true) {
    next_index = PeekNextOn();
    if (
      next_index == kNullIndex ||
      tape_->notes[tape_->head_link.on_index].on_pos >= tape_->notes[next_index].on_pos
    ) {
      break;
    }
    tape_->head_link.on_index = next_index;
  }

  while (true) {
    next_index = PeekNextOff();
    if (
      next_index == kNullIndex ||
      tape_->notes[tape_->head_link.off_index].off_pos >= tape_->notes[next_index].off_pos
    ) {
      break;
    }
    tape_->head_link.off_index = next_index;
  }
}

void Deck::RemoveOldestNote(uint16_t current_pos) {
  RemoveNote(current_pos, tape_->oldest_index);
  if (!IsEmpty()) {
    tape_->oldest_index = stmlib::modulo(tape_->oldest_index + 1, kMaxNotes);
  }
}

void Deck::RemoveNewestNote(uint16_t current_pos) {
  RemoveNote(current_pos, tape_->newest_index);
  if (!IsEmpty()) {
    tape_->newest_index = stmlib::modulo(tape_->newest_index - 1, kMaxNotes);
  }
}

uint8_t Deck::PeekNextOn() const {
  if (tape_->head_link.on_index == kNullIndex) {
    return kNullIndex;
  }
  return tape_->notes[tape_->head_link.on_index].next_link.on_index;
}

uint8_t Deck::PeekNextOff() const {
  if (tape_->head_link.off_index == kNullIndex) {
    return kNullIndex;
  }
  return tape_->notes[tape_->head_link.off_index].next_link.off_index;
}

void Deck::Advance(uint16_t old_pos, uint16_t new_pos) {
  uint8_t seen_index;
  uint8_t next_index;

  seen_index = looper::kNullIndex;
  while (true) {
    next_index = PeekNextOff();
    if (next_index == kNullIndex || next_index == seen_index) {
      break;
    }
    if (seen_index == kNullIndex) {
      seen_index = next_index;
    }
    const Note& next_note = tape_->notes[next_index];
    if (!Passed(next_note.off_pos, old_pos, new_pos)) {
      break;
    }
    tape_->head_link.off_index = next_index;

    part_->LooperPlayNoteOff(next_index, next_note.pitch);
  }

  seen_index = looper::kNullIndex;
  while (true) {
    next_index = PeekNextOn();
    if (next_index == kNullIndex || next_index == seen_index) {
      break;
    }
    if (seen_index == kNullIndex) {
      seen_index = next_index;
    }
    const Note& next_note = tape_->notes[next_index];
    if (!Passed(next_note.on_pos, old_pos, new_pos)) {
      break;
    }
    tape_->head_link.on_index = next_index;

    if (next_note.next_link.off_index == kNullIndex) {
      // If the next 'on' note doesn't yet have an off_index, it's still held
      // and has been for an entire loop -- instead of redundantly turning the
      // note on, set an off_pos
      InsertOff(next_note.on_pos, next_index);
      continue;
    }

    part_->LooperPlayNoteOn(next_index, next_note.pitch, next_note.velocity);
  }
}

uint8_t Deck::RecordNoteOn(uint16_t pos, uint8_t pitch, uint8_t velocity) {
  if (!IsEmpty()) {
    tape_->newest_index = stmlib::modulo(1 + tape_->newest_index, kMaxNotes);
  }
  if (tape_->newest_index == tape_->oldest_index) {
    RemoveOldestNote(pos);
  }

  InsertOn(pos, tape_->newest_index);

  Note& note = tape_->notes[tape_->newest_index];
  note.pitch = pitch;
  note.velocity = velocity;
  note.off_pos = pos;
  note.next_link.off_index = kNullIndex;

  return tape_->newest_index;
}

// Returns whether the NoteOff should be sent
bool Deck::RecordNoteOff(uint16_t pos, uint8_t index) {
  if (tape_->notes[index].next_link.off_index != kNullIndex) {
    // off_pos was already set by Advance, so the note was held for an entire
    // loop, so the note should play continuously and not be turned off now
    return false;
  }
  InsertOff(pos, index);
  return true;
}

bool Deck::NoteIsPlaying(uint8_t index, uint16_t pos) const {
  const Note& note = tape_->notes[index];
  if (note.next_link.off_index == kNullIndex) { return false; }
  return Passed(pos, note.on_pos, note.off_pos);
}

uint16_t Deck::NoteFractionCompleted(uint8_t index, uint16_t pos) const {
  const Note& note = tape_->notes[index];
  uint16_t completed = pos - note.on_pos;
  uint16_t length = note.off_pos - 1 - note.on_pos;
  return (static_cast<uint32_t>(completed) << 16) / length;
}

uint8_t Deck::NotePitch(uint8_t index) const {
  return tape_->notes[index].pitch;
}

uint8_t Deck::NoteAgeOrdinal(uint8_t index) const {
  return stmlib::modulo(index - tape_->oldest_index, kMaxNotes);
}

bool Deck::Passed(uint16_t target, uint16_t before, uint16_t after) const {
  if (before < after) {
    return (target > before and target <= after);
  } else {
    return (target > before or target <= after);
  }
}

void Deck::InsertOn(uint16_t pos, uint8_t index) {
  Note& note = tape_->notes[index];
  note.on_pos = pos;
  if (tape_->head_link.on_index == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    note.next_link.on_index = index;
  } else {
    Note& head_note = tape_->notes[tape_->head_link.on_index];
    note.next_link.on_index = head_note.next_link.on_index;
    head_note.next_link.on_index = index;
  }
  tape_->head_link.on_index = index;
}

void Deck::InsertOff(uint16_t pos, uint8_t index) {
  Note& note = tape_->notes[index];
  note.off_pos = pos;
  if (tape_->head_link.off_index == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    note.next_link.off_index = index;
  } else {
    Note& head_note = tape_->notes[tape_->head_link.off_index];
    note.next_link.off_index = head_note.next_link.off_index;
    head_note.next_link.off_index = index;
  }
  tape_->head_link.off_index = index;
}

void Deck::RemoveNote(uint16_t current_pos, uint8_t target_index) {
  if (IsEmpty()) {
    return;
  }

  Note& target_note = tape_->notes[target_index];
  uint8_t search_prev_index;
  uint8_t search_next_index;

  if (NoteIsPlaying(target_index, current_pos)) {
    part_->LooperPlayNoteOff(target_index, target_note.pitch);
  }

  search_prev_index = target_index;
  while (true) {
    search_next_index = tape_->notes[search_prev_index].next_link.on_index;
    if (search_next_index == target_index) {
      break;
    }
    search_prev_index = search_next_index;
  }
  tape_->notes[search_prev_index].next_link.on_index = target_note.next_link.on_index;
  target_note.next_link.on_index = kNullIndex; // unneeded?
  if (target_index == search_prev_index) {
    // If this was the last note
    tape_->head_link.on_index = kNullIndex;
  } else if (target_index == tape_->head_link.on_index) {
    tape_->head_link.on_index = search_prev_index;
  }

  if (target_note.next_link.off_index == kNullIndex) {
    // Don't try to relink off_index
    return;
  }

  search_prev_index = target_index;
  while (true) {
    search_next_index = tape_->notes[search_prev_index].next_link.off_index;
    if (search_next_index == target_index) {
      break;
    }
    search_prev_index = search_next_index;
  }
  tape_->notes[search_prev_index].next_link.off_index = target_note.next_link.off_index;
  target_note.next_link.off_index = kNullIndex;
  if (target_index == search_prev_index) {
    // If this was the last note
    tape_->head_link.off_index = kNullIndex;
  } else if (target_index == tape_->head_link.off_index) {
    tape_->head_link.off_index = search_prev_index;
  }
}

} // namespace looper
}  // namespace yarns
