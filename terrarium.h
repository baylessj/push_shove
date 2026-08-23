// PedalPCB Terrarium Header
// Copyright (C) 2020 PedalPCB.com
// http://www.pedalpcb.com
#include "daisy_seed.h"


namespace terrarium
{
namespace footswitch
{
    constexpr daisy::Pin left  = daisy::seed::D25;
    constexpr daisy::Pin right = daisy::seed::D26;
} // namespace footswitch

namespace toggleswitch
{
    constexpr uint8_t one   = 10;
    constexpr uint8_t two   = 9;
    constexpr uint8_t three = 8;
    constexpr uint8_t four  = 7;
} // namespace toggleswitch

namespace knob
{
    constexpr uint8_t top_left  = 22;
    constexpr uint8_t top_mid   = 23;
    constexpr uint8_t top_right = 24;
    constexpr uint8_t bot_left  = 25;
    constexpr uint8_t bot_mid   = 26;
    constexpr uint8_t bot_right = 27;
} // namespace knob

namespace led
{
    constexpr daisy::Pin left  = daisy::seed::D22;
    constexpr daisy::Pin right = daisy::seed::D23;
} // namespace led
} // namespace terrarium
