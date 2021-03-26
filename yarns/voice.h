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
// Voice.

#ifndef YARNS_VOICE_H_
#define YARNS_VOICE_H_

#include "stmlib/stmlib.h"
#include "stmlib/utils/ring_buffer.h"

#include "yarns/envelope.h"
#include "yarns/oscillator.h"
#include "yarns/synced_lfo.h"
#include "yarns/part.h"
#include "yarns/clock_division.h"

namespace yarns {

const uint16_t kNumOctaves = 11;

enum TriggerShape {
  TRIGGER_SHAPE_SQUARE,
  TRIGGER_SHAPE_LINEAR,
  TRIGGER_SHAPE_EXPONENTIAL,
  TRIGGER_SHAPE_RING,
  TRIGGER_SHAPE_STEPS,
  TRIGGER_SHAPE_NOISE_BURST
};

enum OscillatorMode {
  OSCILLATOR_MODE_OFF,
  OSCILLATOR_MODE_DRONE,
  OSCILLATOR_MODE_ENVELOPED,

  OSCILLATOR_MODE_LAST
};

enum ModAux {
  MOD_AUX_VELOCITY,
  MOD_AUX_MODULATION,
  MOD_AUX_AFTERTOUCH,
  MOD_AUX_BREATH,
  MOD_AUX_PEDAL,
  MOD_AUX_BEND,
  MOD_AUX_VIBRATO_LFO,
  MOD_AUX_FULL_LFO,
  MOD_AUX_ENVELOPE,

  MOD_AUX_LAST
};

enum DCRole {
  DC_PITCH,
  DC_VELOCITY,
  DC_AUX_1,
  DC_AUX_2,
  DC_TRIGGER,
};

class Voice {
 public:
  Voice() { }
  ~Voice() { }

  void Init();
  void ResetAllControllers();

  void Refresh(uint8_t voice_index);
  void NoteOn(int16_t note, uint8_t velocity, uint8_t portamento, bool trigger);
  void NoteOff();
  void ControlChange(uint8_t controller, uint8_t value);
  void PitchBend(uint16_t pitch_bend) {
    mod_pitch_bend_ = pitch_bend;
  }
  void Aftertouch(uint8_t velocity) {
    mod_aux_[MOD_AUX_AFTERTOUCH] = velocity << 9;
  }

  inline void Clock() {
    if (!modulation_sync_ticks_) { return; }
    synced_lfo_.Tap(modulation_sync_ticks_);
  }
  void set_modulation_rate(uint8_t modulation_rate, uint8_t index);
  inline void set_pitch_bend_range(uint8_t pitch_bend_range) {
    pitch_bend_range_ = pitch_bend_range;
  }
  inline void set_vibrato_range(uint8_t vibrato_range) {
    vibrato_range_ = vibrato_range;
  }
  inline void set_vibrato_initial(uint8_t n) {
    vibrato_initial_ = n;
  }
  inline void set_trigger_duration(uint8_t trigger_duration) {
    trigger_duration_ = trigger_duration;
  }
  inline void set_trigger_scale(uint8_t trigger_scale) {
    trigger_scale_ = trigger_scale;
  }
  inline void set_trigger_shape(uint8_t trigger_shape) {
    trigger_shape_ = trigger_shape;
  }
  inline void set_aux_cv(uint8_t i) { aux_cv_source_ = i; }
  inline void set_aux_cv_2(uint8_t i) { aux_cv_source_2_ = i; }
  inline uint8_t get_aux_cv() { return aux_cv_source_; }
  inline uint8_t get_aux_cv_2() { return aux_cv_source_2_; }
  
  inline int32_t note() const { return note_; }
  inline uint8_t velocity() const { return mod_velocity_; }
  inline uint8_t modulation() const { return mod_wheel_; }
  inline uint16_t aux_cv_16bit() const { return mod_aux_[aux_cv_source_]; }
  inline uint16_t aux_cv_2_16bit() const { return mod_aux_[aux_cv_source_2_]; }
  inline uint8_t aux_cv() const { return aux_cv_16bit() >> 8; }
  inline uint8_t aux_cv_2() const { return aux_cv_2_16bit() >> 8; }
  
  inline bool gate_on() const { return gate_; }

  inline bool gate() const { return gate_ && !retrigger_delay_; }
  inline bool trigger() const  {
    return gate_ && trigger_pulse_;
  }
  
  uint16_t trigger_value() const;
  
  inline void set_oscillator_mode(uint8_t m) {
    oscillator_mode_ = m;
  }
  inline void set_oscillator_shape(uint8_t s) {
    oscillator_.set_shape(static_cast<OscillatorShape>(s));
  }
  inline void set_timbre_init(uint8_t n) {
    timbre_init_target_21_ = n << (21 - 6);
  }
  inline void set_timbre_mod_lfo(uint8_t n) { timbre_mod_lfo_ = n; }
  
  inline void set_tuning(int8_t coarse, int8_t fine) {
    tuning_ = (static_cast<int32_t>(coarse) << 7) + fine;
  }
  
  inline bool has_audio() const {
    return oscillator_mode_ != OSCILLATOR_MODE_OFF;
  }

  inline Oscillator* oscillator() {
    return &oscillator_;
  }

  inline Envelope* envelope() {
    return &envelope_;
  }

  inline void set_timbre_mod_envelope(int16_t n) {
    timbre_mod_envelope_target_ = n;
  }

  inline void RenderSamples() {
    if (has_audio()) { oscillator_.Render(); }
  }
  inline uint16_t ReadSample() {
    return oscillator_.ReadSample();
  }
  
 private:
  // Clock-synced LFO.
  SyncedLFO synced_lfo_;

  int32_t note_source_;
  int32_t note_target_;
  int32_t note_portamento_;
  int32_t note_;
  int32_t tuning_;
  bool gate_;
  
  int16_t mod_pitch_bend_;
  uint8_t mod_wheel_;
  uint16_t mod_aux_[MOD_AUX_LAST];
  uint8_t mod_velocity_;
  
  uint8_t pitch_bend_range_;
  uint32_t modulation_increment_;
  uint16_t modulation_sync_ticks_;
  uint8_t vibrato_range_;
  uint8_t vibrato_initial_;
  
  uint8_t trigger_duration_;
  uint8_t trigger_shape_;
  bool trigger_scale_;
  uint8_t aux_cv_source_;
  uint8_t aux_cv_source_2_;
  
  uint32_t portamento_phase_;
  uint32_t portamento_phase_increment_;
  bool portamento_exponential_shape_;
  
  // This counter is used to artificially create a 750µs (3-systick) dip at LOW
  // level when the gate is currently HIGH and a new note arrive with a
  // retrigger command. This happens with note-stealing; or when sending a MIDI
  // sequence with overlapping notes.
  uint16_t retrigger_delay_;
  
  uint16_t trigger_pulse_;
  uint32_t trigger_phase_increment_;
  uint32_t trigger_phase_;
  
  uint8_t oscillator_mode_;
  int32_t timbre_init_target_21_;
  int32_t timbre_init_current_21_;
  int8_t timbre_mod_lfo_;
  Oscillator oscillator_;
  Envelope envelope_;
  int16_t timbre_mod_envelope_target_;
  int16_t timbre_mod_envelope_current_;

  DISALLOW_COPY_AND_ASSIGN(Voice);
};

class CVOutput {
 public:
  CVOutput() { }
  ~CVOutput() { }

  typedef uint16_t (CVOutput::*DCFn)() const;
  static DCFn dc_fn_table_[];

  void Init(bool reset_calibration);

  void Calibrate(uint16_t* calibrated_dac_code);

  // NB: a voice can supply DC to many CV outputs, but audio to only one output
  inline void assign(Voice* dc, DCRole dc_role, uint8_t num_audio) {
    dc_voice_ = dc;
    dc_role_ = dc_role;
    num_audio_voices_ = num_audio;
    int32_t offset = volts_dac_code(0);
    // Combined audio amplitude 4Vpp
    int32_t scale = (offset - volts_dac_code(4)) / num_audio_voices_;
    for (uint8_t i = 0; i < num_audio_voices_; ++i) {
      Voice* audio_voice = audio_voices_[i] = dc_voice_ + i;
      audio_voice->oscillator()->Init(scale, offset);
    }
  }

  inline bool gate() const {
    if (is_audio()) {
      for (uint8_t i = 0; i < num_audio_voices_; ++i) {
        if (audio_voices_[i]->gate()) { return true; }
      }
      return false;
    } else {
      return dc_voice_->gate();
    }
  }

  inline bool is_AC() const {
    return is_audio() || (
      (dc_role_ == DC_AUX_1 && dc_voice_->get_aux_cv() == MOD_AUX_ENVELOPE) ||
      (dc_role_ == DC_AUX_2 && dc_voice_->get_aux_cv_2() == MOD_AUX_ENVELOPE)
    );
  }

  inline bool is_audio() const {
    return num_audio_voices_ > 0 && audio_voices_[0]->has_audio();
  }

  inline void RenderSamples() {
    for (uint8_t i = 0; i < num_audio_voices_; ++i) {
      audio_voices_[i]->RenderSamples();
    }
  }

  inline uint16_t GetAC() {
    if (is_audio()) {
      uint16_t mix = 0;
      for (uint8_t i = 0; i < num_audio_voices_; ++i) {
        audio_voices_[i]->envelope()->RenderSample();
        mix += audio_voices_[i]->ReadSample();
      }
      return mix;
    } else {
      if (dc_voice_->envelope()->RenderSample()) {
        env_dac_current_ = env_dac_target_;
        env_dac_target_ = DacCodeFrom16BitValue(dc_voice_->envelope()->value());
        env_dac_increment_ = (env_dac_target_ - env_dac_current_) / 24;
      }
      env_dac_current_ += env_dac_increment_;
      return env_dac_current_;
    }
  }

  void Refresh();

  inline uint16_t GetDC() const {
    if (is_AC()) return 0;
    DCFn fn = dc_fn_table_[dc_role_];
    return (this->*fn)();
  }

  // Use a cache/dirty approach for these? or render envelopes via GetAudio?
  inline uint16_t DacCodeFrom16BitValue(uint16_t value) const {
    uint32_t v = static_cast<uint32_t>(value);
    uint32_t scale = volts_dac_code(0) - volts_dac_code(7);
    return static_cast<uint16_t>(volts_dac_code(0) - (scale * v >> 16));
  }

  inline uint16_t note_dac_code() const {
    return note_dac_code_;
  }

  inline uint16_t velocity_dac_code() const {
    return DacCodeFrom16BitValue(dc_voice_->velocity() << 9);
  }
  inline uint16_t aux_cv_dac_code() const {
    return DacCodeFrom16BitValue(dc_voice_->aux_cv_16bit());
  }
  inline uint16_t aux_cv_dac_code_2() const {
    return DacCodeFrom16BitValue(dc_voice_->aux_cv_2_16bit());
  }
  inline uint16_t trigger_dac_code() const {
    int32_t max = volts_dac_code(5);
    int32_t min = volts_dac_code(0);
    return min + ((max - min) * dc_voice_->trigger_value() >> 15);
  }

  inline uint16_t calibration_dac_code(uint8_t note) const {
    return calibrated_dac_code_[note];
  }

  inline void set_calibration_dac_code(uint8_t note, uint16_t dac_code) {
    calibrated_dac_code_[note] = dac_code;
    dirty_ = true;
  }

  inline uint16_t volts_dac_code(int8_t volts) const {
    return calibration_dac_code(volts + 3);
  }

 private:
  void NoteToDacCode();

  Voice* dc_voice_;
  Voice* audio_voices_[kNumMaxVoicesPerPart];
  uint8_t num_audio_voices_;
  DCRole dc_role_;

  int32_t note_;
  uint16_t note_dac_code_;
  bool dirty_;  // Set to true when the calibration settings have changed.
  uint16_t calibrated_dac_code_[kNumOctaves];

  uint16_t env_dac_current_;
  uint16_t env_dac_target_;
  int32_t env_dac_increment_;

  DISALLOW_COPY_AND_ASSIGN(CVOutput);
};

}  // namespace yarns

#endif // YARNS_VOICE_H_
