#include "yin.h"

#include "Utility/dcblock.h"

#include <cmath>

namespace octave {

static const size_t HISTORY_SIZE = 16;
static float _freq_history[HISTORY_SIZE];
static size_t _freq_history_head = 0;

static float _period_history[HISTORY_SIZE];
static size_t _period_history_head = 0;

void _freqPush(float val) {
  _freq_history[_freq_history_head] = val;
  _freq_history_head = (_freq_history_head + 1) % HISTORY_SIZE;
}

void _periodPush(float val) {
  _period_history[_period_history_head] = val;
  _period_history_head = (_period_history_head + 1) % HISTORY_SIZE;
}

float _prevFreq() {
  return _freq_history[(_freq_history_head + HISTORY_SIZE - 1) % HISTORY_SIZE];
}

float _prevPeriod() {
  return _period_history[(_period_history_head + HISTORY_SIZE - 1) %
                         HISTORY_SIZE];
}

static float _fs = 0.0f;

static int _output_sgn = 1;

static float _ema = 0.0f;
static const float EMA_MULT = 0.01;

static float _volume_ema = 0.0f;
static const float VOLUME_EMA_MULT = 0.08;

static float _square_ema = 0.0f;
static const float SQUARE_EMA_MULT = 0.1;

daisysp::DcBlock dc_block;

void init(float sampleRate) {
  _fs = sampleRate;
  dc_block.Init(sampleRate);
  for (int i = 0; i < HISTORY_SIZE; ++i) {
    _freqPush(0.0f);
    _periodPush(0.0f);
  }
}

static float _past_volume = 0.0f;
static size_t _vol_tracker = 0;
static const size_t VOL_TRACKER_PERIOD = 30;
static float _phase = 0.0f;

static constexpr float PITCH_SLEW_NORMAL = 0.2f;
static constexpr float PITCH_SLEW_NEW_NOTE = 1.0f;

static constexpr float NOTE_CHANGE_SEMITONES = 5.0f;

// Require a meaningful increase in volume to detect a new note.
static constexpr float ONSET_VOLUME_RATIO = 1.4f;
static constexpr float ONSET_VOLUME_DELTA = 0.015f;

// Ignore estimates that are too far outside the expected bass range.
static constexpr float MIN_FREQ = 25.0f;
static constexpr float MAX_FREQ = 500.0f;

float stabilizeFreq(float prev_freq, float freq, bool new_note_vol) {
  if (prev_freq <= 0.0f) {
    // no history yet
    return freq;
  } else {
    float ratio = freq / prev_freq;

    bool is_oct_up = ratio > 1.8f && ratio < 2.2f;
    bool is_oct_down = ratio > 0.45f && ratio < 0.55f;
    if (!new_note_vol && (is_oct_up || is_oct_down)) {
      // pitch estimate is likely tracking a harmonic. could be a big slide but
      // unlikely to happen so quickly
      return prev_freq;
    }

    float semitones = 12.0f * log2f(freq / prev_freq);
    float pitch_change = fabs(semitones);

    bool large_pitch_change = pitch_change >= NOTE_CHANGE_SEMITONES;
    if (large_pitch_change && !new_note_vol) {
      return prev_freq;
    }

    float slew = large_pitch_change ? PITCH_SLEW_NEW_NOTE : PITCH_SLEW_NORMAL;
    return prev_freq + (freq - prev_freq) * slew;
  }
}

float process(float x) {
  float in = dc_block.Process(x);
  _ema += (in - _ema) * EMA_MULT;
  _volume_ema += (fabs(_ema) - _volume_ema) * VOLUME_EMA_MULT;

  yin::write(x);
  yin::result yr = yin::read();

  // check for a new note
  bool new_note_vol = _volume_ema > _past_volume * ONSET_VOLUME_RATIO &&
                      (_volume_ema - _past_volume) > ONSET_VOLUME_DELTA;

  if (yr.period <= 0.0f) {
    return 0.0f;
  }

  // stabilize pitch
  float prev_freq = _prevFreq();
  float freq = _fs / yr.period;
  float stable_freq = stabilizeFreq(prev_freq, freq, new_note_vol);
  _freqPush(stable_freq);

  if (stable_freq - prev_freq > 0.01) {
    _periodPush(stable_freq);
  }

  if (stable_freq > 0.0f) {
    float output_freq = stable_freq * 0.5f;

    // Phase accumulator: one complete cycle per
    // output period. Preserving phase avoids clicks
    // when the estimated frequency changes.
    _phase += output_freq / _fs;

    if (_phase >= 1.0f) {
      _phase -= 1.0f;
    }

    _output_sgn = _phase < 0.5f ? 1 : -1;

    float square = _volume_ema * _output_sgn;
    _square_ema += (square - _square_ema) * SQUARE_EMA_MULT;
  }

  // record previous volume every n samples
  if (++_vol_tracker >= VOL_TRACKER_PERIOD) {
    _past_volume = _volume_ema;
    _vol_tracker = 0;
  }

  return _square_ema;
}
} // namespace octave