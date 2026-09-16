# Software and build verification

## Passed

- Both target profiles compile and link successfully. Full enforces nominal 33% carrier and GPIO34 voltage lockout; simple enforces nominal 50% with one low-current emitter. Native battery tests cover startup validity, the 3.40 V trip, 3.55 V release hysteresis, zero/out-of-range readings and recovery. Static integration checks verify profile flags, common carrier override and the guard-before-send ordering.
- Both assembly PDFs are single-page A3 landscape, visually inspected after rendering. Text stays within page bounds. Source diagrams and the board reference are included.

- ESP32 release compile and link with PlatformIO espressif32 6.10.0, official Arduino-ESP32 2.0.17, Xtensa GCC 8.4.0 esp-2021r2-patch5, TFT_eSPI 2.5.43, XPT2046 1.4, and IRremoteESP8266 2.8.6.
- Full profile: linked flash **964,761 / 4,063,232 bytes (23.7%)**. Simple profile: **964,553 / 4,063,232 bytes (23.7%)**. Both use **31,008 / 327,680 bytes (9.5%)** of static RAM. Runtime allocations, stacks, and IR receive buffers are additional; these numbers are not measured peak RAM or battery-life results.
- esptool 4.5.1 validates the application image checksum and SHA-256 digest. Binary partition parsing confirms NVS at 0x9000 (64 KB), PHY data at 0x19000, and factory app at 0x20000 (3,968 KB). All regions fit 4 MB flash.
- Native C++17 behavioral checks pass under AddressSanitizer and UndefinedBehaviorSanitizer; LeakSanitizer was disabled because it cannot operate under this execution environment's process tracing.
- All **2,978 catalog records** pass bounds, unique-ID, string termination, raw-payload bounds, parser, and serialization round-trip checks.
- Application checks cover tree navigation, Favorites tap versus hold, adding favorites, saved-code creation and renaming, eight-slot refusal, persistence failure without state mutation, maximum-sized raw storage, and rejection of every truncated prefix of a saved-state blob.
- Five independent Python conversion tests cover known NEC/LG, Samsung, Sony, RC5/RC6 vectors and invalid raw durations.
- Ten screens are generated through the shared firmware layout, including the power settings screen. The previews use the same 4-bit Inter glyph bitmaps as the device, including measured-width text fitting. This does not validate physical touch, panel color reproduction, or rendering speed on hardware.

## Input handling and efficiency

- Default mode blanks the backlight after 60 seconds. Power saving uses approximately 50% backlight PWM and display/ESP32 light sleep after 20 seconds. The selected mode persists independently of the saved-code storage blob.
- Power tests cover both timeout boundaries, activity resets, busy suppression, millisecond wraparound, held wake gestures, release bounce and power-preference write failure. Receiver lifecycle tests reject stop-before-start and duplicate start/stop transitions before reaching the hardware timer API.
- Sleep entry uses RTC EXT0 on GPIO36, with no XPT2046 digital interrupt handler. The current UI stays in RAM. GPIO27 stays low through sleep and wake. Hardware current and touch-wake behavior require the checks in `POWER_SAVING.md`.

- Touch releases are accepted only on the same displayed view as the initial press. A pending capture/import, changed view, drag, short bounce or wake gesture cannot activate a different button. Timing checks include millisecond-counter wraparound.
- All six saved calibration coefficients must be finite and produce a bounded, nondegenerate mapping; invalid data triggers calibration.
- RC5 and RC6 toggle independently. Unrelated protocols do not alter either toggle state.
- USB processing handles at most 256 bytes per loop, preserving opportunities to poll touch and reception. Oversized input is discarded through its newline and reports an error.
- Catalog tree lists are built when entering a level and reused during paging/redraw. Touch hit testing reads button data by reference. Rendering remains event-driven.
- Regression checks exercise changed-view touches, wake/drag/bounce rejection, counter wrap, invalid calibration coefficients, mixed-protocol toggle sequences and tree navigation.

## Build provenance

The standard `platformio.ini` downloads Espressif dependencies through PlatformIO. The verified build used locally staged copies of the pinned upstream tools: Arduino-ESP32 tag 2.0.17 (`5e19e086c43d0fa5e5a596497ff8f11a0a43f6c2`), official Espressif GCC archive, esptool tag v4.5.1, and SCons 4.11.1. Local staging paths are not part of the portable project configuration.

The compiler archive SHA-256 was checked against Espressif's Arduino package index: `8ef14e0409c2011b41e504a30f70d3e35287313a795d1f2462ad2cd0e2052d37`. `firmware/full/manifest.json` and `firmware/simple/manifest.json` record the image hashes, profile and flash offsets. Both target links passed with nonempty object files and freshly generated library archives.

## Requires physical testing

No E32R28T was connected during development. Display orientation/color, touch calibration accuracy, GPIO output, carrier waveform, receiver sensitivity, replay compatibility, real NVS behavior under power loss, current draw, battery runtime, and range have **not** been verified on hardware. The acceptance sequence is in `INSTALL.md`.

Desktop tests and target compilation verify software behavior and build compatibility; they do not establish physical hardware performance. Catalog entries are community contributions and can contain inaccurate labels or recordings. A demodulating receiver cannot learn the original carrier frequency; unknown signals use 38 kHz.
