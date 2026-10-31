#include "Utility/dcblock.h"

#include <cmath>

namespace octave {
static float _fs = 0.0f;

static int _output_sgn = 1;

static float _ema = 0.0f;
static const float EMA_MULT = 0.01;

static float _volume_ema = 0.0f;
static const float VOLUME_EMA_MULT = 0.08;

static float _square_ema = 0.0f;
static float _square_clipped_ema = 0.0f;
static const float SQUARE_EMA_MULT = 0.1;

static int samples_since_cross = 0;

static const float DEADBAND = 0.001f;
static const int MIN_CROSSING_PERIOD =
    40; // min num samples to count as a zero cross
static int _prev_cross_sign = 0;
static int _count = 0;

daisysp::DcBlock dc_block;

void init(float sampleRate) {
  _fs = sampleRate;
  dc_block.Init(sampleRate);
}

float process(float x) {
  _ema += (x - _ema) * EMA_MULT;
  _volume_ema = (fabs(_ema) - _volume_ema) * VOLUME_EMA_MULT + _volume_ema;

  if (_volume_ema >= 0.0001) {
    // only handle crossings when volume is useful

    // TODO: use volume envelope for proxy for changing note? like if
    // the volume has been decreasing for a while, then try to stick to
    // the current note. would mess with slides though
    samples_since_cross++;

    int sign = 0;
    if (_ema >= DEADBAND) {
      sign = 1;
    } else if (_ema <= -DEADBAND) {
      sign = -1;
    }

    if (sign != 0 && _prev_cross_sign != 0 && sign != _prev_cross_sign &&
        samples_since_cross >= MIN_CROSSING_PERIOD) {
      _prev_cross_sign = sign;
      samples_since_cross = 0;
      _count++;
    } else if (sign != 0 && _prev_cross_sign == 0) {
      // init
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
  // float lpf_result = lpf2.Process(_square_ema);
  // return   square;
  // return dc_block.Process(square);
  return _square_ema;
  // return clipped;
  // return _square_clipped_ema;
} // namespace octave
} // namespace octave