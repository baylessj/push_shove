#include <string.h>
#include <math.h>
#include "Utility/dsp.h"
#include "daisy_seed.h"
#include "terrarium.h"

using namespace daisy;

DaisySeed hw;

bool bypass = true;

Switch footswitch;
Led    led_left;

// range of [-50, 50]
const float curve = 0;

static float max_high_in  = 0.0;
static float max_low_in   = 0.0;
static float max_high_out = 0.0;
static float max_low_out  = 0.0;

static void Callback(AudioHandle::InterleavingInputBuffer  in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t                                size)
{
    footswitch.Debounce();
    led_left.Update();

    if(footswitch.RisingEdge())
    {
        bypass = !bypass;
        led_left.Set(bypass ? 0.0f : 1.0f);
    }

    if(bypass)
    {
        memcpy(out, in, size * sizeof(float));
    }
    else
    {
        float a = curve * 0.01 + 1.5;
        float b = curve * -0.02;
        float c = curve * 0.01 - 0.5;
        float d = 0.0625 - curve * 0.0025 + sqrtf(curve) * 0.000025;

        for(size_t i = 0; i < size; i += 2)
        {
            float x     = in[i] * 5; // x6 to scale up to [-1, 1] roughly
            float abs_x = fabs(x);
            float sqr_x = x * x;

            float tmp = 0.0;
            if(abs_x < 1.0)
            {
                // ax + bx^2 + cx+3 + d(x^2 - x^3 + x^4)
                tmp = a * abs_x + b * sqr_x + c * abs_x * sqr_x
                      - d * (sqr_x - 2 * sqr_x * abs_x + sqr_x * sqr_x);
            }
            else
            {
                tmp = 2 * abs_x - x * x;
            }
            int sgn_x = x > 0 ? 1 : -1;
            tmp *= sgn_x;

            if(x > max_high_in)
            {
                max_high_in = x;
            }
            if(x < max_low_in)
            {
                max_low_in = x;
            }
            if(tmp > max_high_out)
            {
                max_high_out = tmp;
            }
            if(tmp < max_low_out)
            {
                max_low_out = tmp;
            }
            out[i] = tmp * 0.12; // scale back down to inst level
        }
        // memcpy(out, 0, size * sizeof(float));
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);

    // set button to pin 4, update at 1kHz
    footswitch.Init(terrarium::footswitch::left);
    led_left.Init(terrarium::led::left, false);

    hw.StartAudio(Callback);
    while(1) {}
}
