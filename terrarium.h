// PedalPCB Terrarium Header
// Copyright (C) 2020 PedalPCB.com
// http://www.pedalpcb.com
#include "daisy_seed.h"

namespace terrarium {
namespace footswitch {
constexpr daisy::Pin left = daisy::seed::D25;
constexpr daisy::Pin right = daisy::seed::D26;
} // namespace footswitch

namespace toggle {
constexpr daisy::Pin one = daisy::seed::D9;
constexpr daisy::Pin two = daisy::seed::D8;
constexpr daisy::Pin three = daisy::seed::D7;
constexpr daisy::Pin four = daisy::seed::D6;
} // namespace toggle

namespace knob {
constexpr daisy::Pin top_left = daisy::seed::A1;
constexpr daisy::Pin top_mid = daisy::seed::A2;
constexpr daisy::Pin top_right = daisy::seed::A3;
constexpr daisy::Pin bot_left = daisy::seed::A4;
constexpr daisy::Pin bot_mid = daisy::seed::A5;
constexpr daisy::Pin bot_right = daisy::seed::A6;
} // namespace knob

namespace led {
constexpr daisy::Pin left = daisy::seed::D22;
constexpr daisy::Pin right = daisy::seed::D23;
} // namespace led
} // namespace terrarium
