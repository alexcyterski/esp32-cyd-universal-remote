#pragma once
namespace board {
#ifndef PULSE_FULL_BUILD
#define PULSE_FULL_BUILD 0
#endif
constexpr bool FULL_BUILD=PULSE_FULL_BUILD;
constexpr int BATTERY_ADC=34;
// E32R28T onboard 100k / 100k divider. Adjust after comparison with a meter.
constexpr float BATTERY_SCALE=2.0f;
// E32R28T only; not the older ESP32-2432S028R.
constexpr int IR_TX = 27, IR_RX = 35, BACKLIGHT = 21;
constexpr int TOUCH_CLK = 25, TOUCH_MISO = 39, TOUCH_MOSI = 32;
constexpr int TOUCH_CS = 33, TOUCH_IRQ = 36;
constexpr int DISPLAY_ROTATION = 0;
constexpr int LEARN_TIMEOUT_MS = 12000;
constexpr int RAW_CARRIER_KHZ = 38; // Demodulating receivers cannot measure carrier.
constexpr int MAX_SAVED = 8, MAX_FAVORITES = 64, MAX_RAW = 1024;
}
