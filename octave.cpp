#include "Utility/dcblock.h"

#include <cmath>

namespace octave {
static float _fs = 0.0f;

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

static int _output_sgn = 1;

static float _ema = 0.0f;
static const float EMA_MULT = 0.006;

static float _volume_ema = 0.0f;
static const float VOLUME_EMA_MULT = 0.08;

static float _square_ema = 0.0f;
static float _square_clipped_ema = 0.0f;
static const float SQUARE_EMA_MULT = 0.1;

static int samples_since_cross = 0;
static int prev_samples_since_cross = 0;

static const float DEADBAND = 0.001f;
// min num samples to count as a zero cross
static const int MIN_CROSSING_PERIOD = 40;
static int _prev_cross_sign = 0;
static int _count = 0;

static float _old_vol = 0.0f;
static int _old_vol_tracker = 0;

daisysp::DcBlock dc_block;

void init(float sampleRate) {
  _fs = sampleRate;
  dc_block.Init(sampleRate);
}

float process(float x) {
  _ema += (x - _ema) * EMA_MULT;
  _volume_ema = (fabs(_ema) - _volume_ema) * VOLUME_EMA_MULT + _volume_ema;

  if (++_old_vol_tracker >= 100) {
    _old_vol = _volume_ema;
    _old_vol_tracker = 0;
  }

  if (_volume_ema >= 0.0001) {
    // only handle crossings when volume is useful
    samples_since_cross++;

    int sign = 0;
    if (_ema >= DEADBAND) {
      sign = 1;
    } else if (_ema <= -DEADBAND) {
      sign = -1;
    }

    bool below_max_freq = samples_since_cross >= MIN_CROSSING_PERIOD;

    int octave_up_diff = samples_since_cross - (prev_samples_since_cross / 2);
    bool near_octave_up = fabs(octave_up_diff) < 50;

    int two_octave_up_diff =
        samples_since_cross - (prev_samples_since_cross / 4);
    bool near_two_octave_up = fabs(two_octave_up_diff) < 30;

    bool new_note_volume = _volume_ema - _old_vol > 0.07;

    bool valid_pitch_change =
        below_max_freq &&
        (new_note_volume || (!near_octave_up && !near_two_octave_up));

    if (sign != 0 && _prev_cross_sign != 0 && sign != _prev_cross_sign &&
        valid_pitch_change) {
      if (valid_pitch_change &&
          fabs(samples_since_cross - prev_samples_since_cross) > 100) {
        _periodPush(samples_since_cross);
      }

      _prev_cross_sign = sign;
      prev_samples_since_cross = samples_since_cross;
      samples_since_cross = 0;
      _count++;
    } else if (sign != 0 && _prev_cross_sign == 0) {
      // init
      prev_samples_since_cross = samples_since_cross;
      _prev_cross_sign = sign;
    }

    if (_count >= 2) {
      // two crossings means we make one (half frequency)
      _output_sgn = _output_sgn == 1 ? -1 : 1;
      _count = 0;
    }
  }

  float square = _volume_ema * _output_sgn;
  _square_ema = (square - _square_ema) * SQUARE_EMA_MULT + _square_ema;
  float clipped = 2 * (fabs(_square_ema)) - _square_ema * _square_ema;
  _square_clipped_ema =
      (clipped - _square_clipped_ema) * SQUARE_EMA_MULT + _square_clipped_ema;
  return _square_ema;
} // namespace octave
} // namespace octave