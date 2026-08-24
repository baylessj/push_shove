#include "Filters/svf.h"
#include "Utility/dsp.h"
#include "daisy_seed.h"
#include "terrarium.h"
#include <math.h>
#include <string.h>

using namespace daisy;

DaisySeed hw;

Switch footswitch;
Led led_left;
GPIO bandsplit_toggle;
enum Pot { pot_input_trim = 0, pot_curve, pot_output_trim, NUM_ADC_CHANNELS };

/**
 * Settings
 */
static bool bypass = true;
static bool bandsplit_enabled = false;
// range of [-50, 50]
static float curve = 0;
float a = 0.0f;
float b = 0.0f;
float c = 0.0f;
float d = 0.0f;
// range of []
static float input_scale = 0;
// range of []
static float output_scale = 0;

/**
 * I/O range checks - debug only
 */
static float max_high_in = 0.0;
static float max_low_in = 0.0;
static float max_high_out = 0.0;
static float max_low_out = 0.0;

/**
 * Band Splitting
 */
daisysp::Svf svf_low, svf_high;
// static params assuming 240/2400 cutoffs
// tan(pi * (cutoff_freq / sample_rate - 0.25)) * 0.5 + 0.5
// float a_c = 0.0154663; // 240Hz
// float b_c = 0.1357287; // 2400Hz
// b_c * (1 - a_c) / (b_c - a_c)
float bandsplit_mid_gain = 0.126616;
// 1 / mid_gain
float bandsplit_mid_gain_div = 7.897896;

struct BandsplitOutputs {
  float low;
  float mid;
  float high;
};

static BandsplitOutputs split_bands(float in) {
  return {
      svf_low.Low(),
      in - svf_low.Low() - svf_high.High(),
      svf_high.High(),
  };
}

/**
 * Waveshaping
 */
static float waveshape(float x) {
  int sgn_x = x > 0 ? 1 : -1;
  float abs_x = fabs(x);
  float sqr_x = x * x;

  float y = 0.0;
  if (abs_x < 1.0) {
    // ax + bx^2 + cx+3 + d(x^2 - x^3 + x^4)
    y = a * abs_x + b * sqr_x + c * abs_x * sqr_x -
        d * (sqr_x - 2 * sqr_x * abs_x + sqr_x * sqr_x);
  } else {
    y = 2 * abs_x - x * x;
  }

  y *= sgn_x;

  // debug
  if (x > max_high_in) {
    max_high_in = x;
  }
  if (x < max_low_in) {
    max_low_in = x;
  }
  if (y > max_high_out) {
    max_high_out = y;
  }
  if (y < max_low_out) {
    max_low_out = y;
  }

  return y;
}

static void Callback(AudioHandle::InterleavingInputBuffer in,
                     AudioHandle::InterleavingOutputBuffer out, size_t size) {
  footswitch.Debounce();
  led_left.Update();

  if (footswitch.RisingEdge()) {
    bypass = !bypass;
    led_left.Set(bypass ? 0.0f : 1.0f);
  }

  if (bypass) {
    memcpy(out, in, size * sizeof(float));
  } else {
    for (size_t i = 0; i < size; i += 2) {
      float x = in[i] * input_scale;
      svf_low.Process(x);
      svf_high.Process(x);

      float y = 0.0f;
      if (bandsplit_enabled) {
        BandsplitOutputs x_split = split_bands(x);
        y = waveshape(x_split.low) +
            waveshape(x_split.mid * bandsplit_mid_gain) *
                bandsplit_mid_gain_div +
            waveshape(x_split.high);
      } else {
        y = waveshape(x);
      }

      out[i] = y * output_scale; // scale back down to inst level
    }
  }
}

int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(4);

  daisy::AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
  adcConfig[pot_input_trim].InitSingle(terrarium::knob::top_left);
  adcConfig[pot_curve].InitSingle(terrarium::knob::top_mid);
  adcConfig[pot_output_trim].InitSingle(terrarium::knob::top_right);
  hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);
  hw.adc.Start();

  footswitch.Init(terrarium::footswitch::left);
  led_left.Init(terrarium::led::left, false);
  bandsplit_toggle.Init(terrarium::toggle::one, GPIO::Mode::INPUT);

  svf_low.Init(hw.AudioSampleRate());
  svf_low.SetFreq(240);
  svf_high.Init(hw.AudioSampleRate());
  svf_high.SetFreq(2400);

  hw.StartAudio(Callback);

  while (1) {
    curve = daisysp::fmap(hw.adc.GetFloat(pot_curve), -50, 50);
    a = curve * 0.01 + 1.5;
    b = curve * -0.02;
    c = curve * 0.01 - 0.5;
    d = .0625 - curve * 0.0025 + (curve * curve * 0.000025);

    // TODO: remap this I/O trim concept to
    // - output boost
    // - I/O coupled "gain" knob that pulls input down and output up in equal
    // measure for similar vol, or vice versa
    input_scale = daisysp::fmap(hw.adc.GetFloat(pot_input_trim), 0.75, 6);
    output_scale = daisysp::fmap(hw.adc.GetFloat(pot_output_trim), 0.05, 1);

    bandsplit_enabled = bandsplit_toggle.Read();

    // other 3 knobs
    // - wet/dry
    // - extra compression stage?
    // - wavefolder?

    daisy::System::Delay(50);
  }
}
