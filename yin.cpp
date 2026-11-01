#include "yin.h"

namespace yin {
static const size_t BUF_SIZE = 3200;
static const size_t MAX_TAU = 1600;
static float _buffer[BUF_SIZE];
static size_t _buf_ptr = 0;

static const size_t MAX_FREQ_SAMPLES = 120;  // 400Hz at 48kHz fs
static const size_t MIN_FREQ_SAMPLES = 1600; // 30Hz at 48kHz fs

static const float THRESHOLD = 0.1; // tune

static float _analysis_buffer[BUF_SIZE];
static float _diff[MAX_TAU + 1];

static result _result;

void write(float in) {
  _buffer[_buf_ptr] = in;
  _buf_ptr = (_buf_ptr + 1) % BUF_SIZE;
}

float bufRead(size_t offset) {
  return _buffer[(_buf_ptr + BUF_SIZE - offset % BUF_SIZE) % BUF_SIZE];
}

bool copyAnalysisBuffer() {
  float sum = 0.0f;
  bool quiet = false;
  for (size_t i = 0; i < BUF_SIZE; ++i) {
    float val = bufRead(BUF_SIZE - 1 - i);
    _analysis_buffer[i] = val;
    sum += val;
    if (i == 300 && sum <= 3.0f) {
      quiet = true;
    }
  }
  return quiet;
}

result read() { return _result; }

result estimate() {
  _diff[0] = 0.0f;

  bool quiet = copyAnalysisBuffer();
  if (quiet) {
    // don't do processing when no input
    return _result;
  }

  // ASDF - squared difference between original + lagged copy
  for (size_t lag = 1; lag <= MIN_FREQ_SAMPLES; ++lag) {
    float sum = 0.0f;
    size_t count = BUF_SIZE - lag;

    for (size_t i = 0; i < count; ++i) {
      float delta = _analysis_buffer[i] - _analysis_buffer[i + lag];
      sum += delta * delta;
    }

    _diff[lag] = sum;
  }

  // YIN's cumulative mean normalized difference
  // the average difference = 1, normalized diff close to 0 means we've found
  // the frequency
  float cumulative = 0.0f;
  for (size_t lag = 1; lag <= MIN_FREQ_SAMPLES; ++lag) {
    cumulative += _diff[lag];

    if (cumulative > 1e-20f) {
      _diff[lag] = _diff[lag] * lag / cumulative;
    } else {
      _diff[lag] = 1.0f;
    }
  }

  // start finding candidates *below* the max frequency, after n samples
  // size_t tau = MIN_FREQ_SAMPLES;
  // while (tau >= MAX_FREQ_SAMPLES && _diff[tau] >= THRESHOLD)
  //   --tau;

  size_t tau = MAX_FREQ_SAMPLES;
  for (size_t i = MAX_FREQ_SAMPLES; i <= MIN_FREQ_SAMPLES; ++i) {
    if (_diff[i] < THRESHOLD && _diff[i] <= _diff[i - 1] &&
        _diff[i] <= _diff[i + 1]) {
      tau = i;
      break;
    }
  }

  // fix if the candidate is an octave too high
  size_t longer_tau = tau * 2;
  if (longer_tau <= MIN_FREQ_SAMPLES) {
    const float octave_thresh = 0.1f;

    if (_diff[longer_tau] < THRESHOLD &&
        _diff[longer_tau] <= _diff[tau] + octave_thresh) {
      tau = longer_tau;
    }
  }

  _result = {tau, _diff[tau]};
  return _result;
}
} // namespace yin