# Power modes and touch wake

Choose a mode in **Listen / Emit → Eco**. Default uses full backlight drive and blanks it after 60 seconds. Power saving uses 128/255 PWM drive (approximately 50%) and sleeps the display and ESP32 after 20 seconds. Both modes consume the waking tap, so waking over an Emit or Favorites button does not send a command.

## Sleep sequence

1. Wait for inactivity and for learning, touch input and queued USB input to finish. Synchronous IR sends and flash writes finish before the main loop can request sleep. An abandoned partial USB line is discarded at the idle boundary.
2. Confirm the touch IRQ is high, then arm active-low RTC EXT0 wake on GPIO36. If configuration fails, remain awake and show an error.
3. Stop reception if running, drive GPIO27 low, turn off the backlight, and send ILI9341 display-off and sleep-in commands. Allow 120 ms for the panel transition.
4. Recheck touch and queued USB input before entering ESP32 light sleep. A touch during panel shutdown cancels CPU sleep and restores the display. There is no periodic timer wake.
5. On wake, disable the wake source, restore GPIO36 to digital input, keep GPIO27 low, and wake the panel with a 120 ms settling delay. Redraw the current view before restoring brightness. Consume the wake gesture until contact has been absent for 50 ms.

The XPT2046 driver is polled while awake and ends conversions with its low-power/PENIRQ-enabled command. Its digital GPIO interrupt handler is not installed; RTC EXT0 handles sleep wake. The E32R28T schematic provides the touch IRQ connection and pull-up. GPIO36 has no internal pull-up, so this implementation must not be assumed to work on a differently wired CYD board.

Backlight PWM uses Arduino-ESP32 2.0.17's LEDC channel 7, 5 kHz, 8-bit resolution. It is independent of IR carrier generation. CPU clock and IR duty profiles are not changed. The `eco` preference is a separate NVS key; Favorites, saved codes and calibration keep their existing storage layout. A failed preference write leaves the selected mode unchanged.

Light sleep preserves RAM and the current UI, including unsaved captures and editor text. Default mode leaves the CPU running when the backlight is off. Neither mode turns off the board's 3.3 V supply, touch controller or externally powered IR receiver. Tap to wake before sending USB commands in Power saving; neither USB nor received IR wakes it. Reset remains available if the touch hardware is faulty.

## Hardware acceptance

- Check the Default timeout at 60 seconds and the Power saving timeout at 20 seconds, measured from the last interaction. Confirm learning stays awake.
- Sleep from Favorites, an action, an unsaved capture and an unfinished description. Tap to wake and verify the same content remains.
- Wake over an Emit/Favorites button using both a short tap and a held touch. Neither should transmit. After release, the next deliberate tap should work.
- Repeat sleep/wake at least 20 times. Include a touch as the screen turns off, then verify display orientation, full/simple IR transmission, learning and the full profile's voltage guard.
- Check GPIO36 is high when released and low when pressed. Scope GPIO27 through sleep and wake to confirm there is no unintended carrier burst.
- Compare battery current at full brightness, reduced brightness, Default screen-off and Power saving sleep. Measure on battery with USB disconnected. Use the measured board current to estimate runtime; bare ESP32 sleep figures do not describe the whole remote.

Firmware builds and desktop tests verify software behavior. No physical E32R28T was connected for current, PWM or touch-wake measurements.

## References

- [Espressif ESP-IDF 4.4.7 sleep modes: light sleep, EXT0 and GPIO restoration](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/sleep_modes.html)
- [E32R28T schematic: touch IRQ and backlight circuit](https://www.lcdwiki.com/res/E32R28T/2.8inch_ESP32-32_Display_Schematic.pdf)
- Bundled `lib/XPT2046_Touchscreen/XPT2046_Touchscreen.cpp`: conversion power-down and polling behavior.
- Bundled `lib/TFT_eSPI/TFT_Drivers/ILI9341_Defines.h`: display sleep command values.
