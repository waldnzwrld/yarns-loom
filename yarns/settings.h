// Copyright 2013 Emilie Gillet.
// Copyright 2020 Chris Rogers.
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
// Parameter definitions.

#ifndef YARNS_SETTINGS_H_
#define YARNS_SETTINGS_H_

#include "stmlib/stmlib.h"

namespace yarns {

enum SettingDomain {
  SETTING_DOMAIN_MULTI,
  SETTING_DOMAIN_PART,
};

enum SettingUnit {
  SETTING_UNIT_UINT8,
  SETTING_UNIT_INT8,
  SETTING_UNIT_INDEX,
  SETTING_UNIT_TEMPO,
  SETTING_UNIT_CLOCK_DIV,
  SETTING_UNIT_MIDI_CHANNEL,
  SETTING_UNIT_MIDI_CHANNEL_OFF,
  SETTING_UNIT_BAR_DURATION,
  SETTING_UNIT_VIBRATO_SPEED,
  SETTING_UNIT_PORTAMENTO,
  SETTING_UNIT_ENUMERATION,
  SETTING_UNIT_ARP_PATTERN,
  SETTING_UNIT_LOOP_LENGTH,
  SETTING_UNIT
};

enum SettingIndex {
  SETTING_MENU_SETUP,
  SETTING_MENU_ENVELOPE,

  SETTING_LAYOUT,
  SETTING_CLOCK_TEMPO,
  SETTING_CLOCK_SWING,
  SETTING_CLOCK_INPUT_DIVISION,
  SETTING_CLOCK_OUTPUT_DIVISION,
  SETTING_CLOCK_BAR_DURATION,
  SETTING_CLOCK_NUDGE_FIRST_TICK,
  SETTING_CLOCK_MANUAL_START,
  SETTING_CLOCK_OVERRIDE,
  SETTING_MIDI_CHANNEL,
  SETTING_MIDI_MIN_NOTE,
  SETTING_MIDI_MAX_NOTE,
  SETTING_MIDI_NOTE,
  SETTING_MIDI_MIN_VELOCITY,
  SETTING_MIDI_MAX_VELOCITY,
  SETTING_MIDI_OUT_MODE,
  SETTING_MIDI_TRANSPOSE_OCTAVES,
  SETTING_VOICING_ALLOCATION_MODE,
  SETTING_VOICING_ALLOCATION_PRIORITY,
  SETTING_VOICING_PORTAMENTO,
  SETTING_VOICING_LEGATO_MODE,
  SETTING_VOICING_PITCH_BEND_RANGE,
  SETTING_VOICING_VIBRATO_RANGE,
  SETTING_VOICING_MODULATION_RATE,
  SETTING_VOICING_VIBRATO_INITIAL,
  SETTING_VOICING_TUNING_TRANSPOSE,
  SETTING_VOICING_TUNING_FINE,
  SETTING_VOICING_TUNING_ROOT,
  SETTING_VOICING_TUNING_SYSTEM,
  SETTING_VOICING_TRIGGER_DURATION,
  SETTING_VOICING_TRIGGER_SCALE,
  SETTING_VOICING_TRIGGER_SHAPE,
  SETTING_VOICING_CV_OUT,
  SETTING_VOICING_CV_OUT_3,
  SETTING_VOICING_CV_OUT_4,
  SETTING_VOICING_OSCILLATOR_SHAPE,
  SETTING_VOICING_OSCILLATOR_PW_INITIAL,
  SETTING_VOICING_OSCILLATOR_PW_MOD,
  SETTING_VOICING_ENVELOPE_AMPLITUDE_INIT,
  SETTING_VOICING_ENVELOPE_AMPLITUDE_MOD,
  SETTING_VOICING_ENV_INIT_ATTACK,
  SETTING_VOICING_ENV_INIT_DECAY,
  SETTING_VOICING_ENV_INIT_SUSTAIN,
  SETTING_VOICING_ENV_INIT_RELEASE,
  SETTING_VOICING_ENV_MOD_ATTACK,
  SETTING_VOICING_ENV_MOD_DECAY,
  SETTING_VOICING_ENV_MOD_SUSTAIN,
  SETTING_VOICING_ENV_MOD_RELEASE,
  SETTING_SEQUENCER_CLOCK_DIVISION,
  SETTING_SEQUENCER_GATE_LENGTH,
  SETTING_SEQUENCER_ARP_RANGE,
  SETTING_SEQUENCER_ARP_DIRECTION,
  SETTING_SEQUENCER_ARP_PATTERN,
  SETTING_SEQUENCER_RHYTHM_PATTERN,  // Alias for arp pattern
  SETTING_SEQUENCER_EUCLIDEAN_LENGTH,
  SETTING_SEQUENCER_EUCLIDEAN_FILL,
  SETTING_SEQUENCER_EUCLIDEAN_ROTATE,
  SETTING_SEQUENCER_PLAY_MODE,
  SETTING_MIDI_INPUT_RESPONSE,
  SETTING_SEQUENCER_CLOCK_QUANTIZATION,
  SETTING_SEQUENCER_LOOP_LENGTH,
  SETTING_MIDI_SUSTAIN_MODE,
  SETTING_MIDI_SUSTAIN_POLARITY,
  SETTING_REMOTE_CONTROL_CHANNEL,
  SETTING_VOICING_TUNING_FACTOR,

  SETTING_LAST,
};

struct Setting {
  const char short_name[3];
  const char* const name;
  SettingDomain domain;
  uint16_t address[2];
  SettingUnit unit;
  int16_t min_value;
  int16_t max_value;
  const char* const* values;
  uint8_t part_cc;
  uint8_t remote_control_cc;
};

class Settings {
 public:
  Settings() { }
  ~Settings() { }

  uint8_t part_cc_map[128];
  uint8_t remote_control_cc_map[128];
  
  void Init();
  
  void Print(const Setting& setting, uint8_t value, char* buffer) const;
  
  inline const Setting& get(uint8_t index) const {
    return settings_[index];
  }
  
  static void PrintInteger(char* buffer, uint8_t number);
  static void PrintSignedInteger(char* buffer, int8_t number);
  
 private:
   
  static const Setting settings_[SETTING_LAST];
  
  DISALLOW_COPY_AND_ASSIGN(Settings);
};

extern Settings setting_defs;

}  // namespace yarns

#endif // YARNS_SETTINGS_H_
