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

#include "yarns/clock_division.h"
#include "yarns/part.h"

namespace yarns {

namespace looper {

void Deck::Init(Part* part) {
  part_ = part;
  tape_ = &(part->mutable_sequencer_settings()->looper_tape);
  RemoveAll();
  Rewind();
}

void Deck::RemoveAll() {
  std::fill(
    &tape_->notes[0],
    &tape_->notes[kMaxNotes],
    Note()
  );
  head_.on = kNullIndex;
  head_.off = kNullIndex;
  tape_->oldest_index = 0;
  tape_->size = 0;
}

void Deck::Rewind() {
  lfo_.Init();
  Advance(0, false);
}

void Deck::RebuildLinks() {
  for (uint8_t i = 0; i < tape_->size; ++i) {
    uint8_t index = index_mod(tape_->oldest_index + i);
    Note& note = tape_->notes[index];
    Advance(note.on_pos, false);
    LinkOn(index);
    Advance(note.off_pos, false);
    LinkOff(index);
  }
}

void Deck::Clock() {
  SequencerSettings seq = part_->sequencer_settings();
  uint16_t num_ticks = clock_division::list[seq.clock_division].num_ticks;
  lfo_.Tap(num_ticks * seq.loop_length);
}

void Deck::RemoveOldestNote() {
  RemoveNote(tape_->oldest_index);
  if (tape_->size) {
    tape_->oldest_index = index_mod(tape_->oldest_index + 1);
  }
}

void Deck::RemoveNewestNote() {
  RemoveNote(index_mod(tape_->oldest_index + tape_->size - 1));
}

uint8_t Deck::PeekNextOn() const {
  if (head_.on == kNullIndex) {
    return kNullIndex;
  }
  return next_link_[head_.on].on;
}

uint8_t Deck::PeekNextOff() const {
  if (head_.off == kNullIndex) {
    return kNullIndex;
  }
  return next_link_[head_.off].off;
}

void Deck::Advance(uint16_t new_pos, bool play) {
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
    if (!Passed(next_note.off_pos, pos_, new_pos)) {
      break;
    }
    head_.off = next_index;

    if (play) {
      part_->LooperPlayNoteOff(next_index, next_note.pitch);
    }
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
    if (!Passed(next_note.on_pos, pos_, new_pos)) {
      break;
    }
    head_.on = next_index;

    if (next_link_[next_index].off == kNullIndex) {
      // If the next 'on' note doesn't yet have an off link, it's still held
      // and has been for an entire loop -- instead of redundantly turning the
      // note on, link its off
      LinkOff(next_index);
      continue;
    }

    if (play) {
      part_->LooperPlayNoteOn(next_index, next_note.pitch, next_note.velocity);
    }
  }

  pos_ = new_pos;
  needs_advance_ = false;
}

uint8_t Deck::RecordNoteOn(uint8_t pitch, uint8_t velocity) {
  if (tape_->size == kMaxNotes) {
    RemoveOldestNote();
  }
  uint8_t index = index_mod(tape_->oldest_index + tape_->size);

  LinkOn(index);
  Note& note = tape_->notes[index];
  note.pitch = pitch;
  note.velocity = velocity;
  note.on_pos = pos_;
  note.off_pos = pos_; // TODO wtf
  next_link_[index].off = kNullIndex;
  tape_->size++;

  return index;
}

// Returns whether the NoteOff should be sent
bool Deck::RecordNoteOff(uint8_t index) {
  if (next_link_[index].off != kNullIndex) {
    // off link was already set by Advance, so the note was held for an entire
    // loop, so the note should play continuously and not be turned off now
    return false;
  }
  LinkOff(index);
  tape_->notes[index].off_pos = pos_;
  return true;
}

bool Deck::NoteIsPlaying(uint8_t index) const {
  const Note& note = tape_->notes[index];
  if (next_link_[index].off == kNullIndex) { return false; }
  return Passed(pos_, note.on_pos, note.off_pos);
}

uint16_t Deck::NoteFractionCompleted(uint8_t index) const {
  const Note& note = tape_->notes[index];
  uint16_t completed = pos_ - note.on_pos;
  uint16_t length = note.off_pos - 1 - note.on_pos;
  return (static_cast<uint32_t>(completed) << 16) / length;
}

uint8_t Deck::NotePitch(uint8_t index) const {
  return tape_->notes[index].pitch;
}

uint8_t Deck::NoteAgeOrdinal(uint8_t index) const {
  return index_mod(index - tape_->oldest_index);
}

bool Deck::Passed(uint16_t target, uint16_t before, uint16_t after) const {
  if (before < after) {
    return (target > before and target <= after);
  } else {
    return (target > before or target <= after);
  }
}

void Deck::LinkOn(uint8_t index) {
  if (head_.on == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    next_link_[index].on = index;
  } else {
    next_link_[index].on = next_link_[head_.on].on;
    next_link_[head_.on].on = index;
  }
  head_.on = index;
}

void Deck::LinkOff(uint8_t index) {
  if (head_.off == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    next_link_[index].off = index;
  } else {
    next_link_[index].off = next_link_[head_.off].off;
    next_link_[head_.off].off = index;
  }
  head_.off = index;
}

void Deck::RemoveNote(uint8_t target_index) {
  // Though this takes an arbitrary index, other methods like NoteAgeOrdinal
  // assume that notes are stored sequentially in memory, so removing a "middle"
  // note will cause problems
  if (!tape_->size) {
    return;
  }

  tape_->size--;
  uint8_t search_prev_index;
  uint8_t search_next_index;

  Note& target_note = tape_->notes[target_index];
  if (NoteIsPlaying(target_index)) {
    part_->LooperPlayNoteOff(target_index, target_note.pitch);
  }

  search_prev_index = target_index;
  while (true) {
    search_next_index = next_link_[search_prev_index].on;
    if (search_next_index == target_index) {
      break;
    }
    search_prev_index = search_next_index;
  }
  next_link_[search_prev_index].on = next_link_[target_index].on;
  next_link_[target_index].on = kNullIndex; // unneeded?
  if (target_index == search_prev_index) {
    // If this was the last note
    head_.on = kNullIndex;
  } else if (target_index == head_.on) {
    head_.on = search_prev_index;
  }

  if (next_link_[target_index].off == kNullIndex) {
    // Don't try to relink off
    return;
  }

  search_prev_index = target_index;
  while (true) {
    search_next_index = next_link_[search_prev_index].off;
    if (search_next_index == target_index) {
      break;
    }
    search_prev_index = search_next_index;
  }
  next_link_[search_prev_index].off = next_link_[target_index].off;
  next_link_[target_index].off = kNullIndex;
  if (target_index == search_prev_index) {
    // If this was the last note
    head_.off = kNullIndex;
  } else if (target_index == head_.off) {
    head_.off = search_prev_index;
  }
}

} // namespace looper
}  // namespace yarns
