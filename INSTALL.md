# Build and install

Target: **E32R28T, ESP32-WROOM-32E, 4 MB flash, ILI9341, XPT2046**. The firmware pin map is not for every board sold as “CYD.”

**For the battery readout and 24 saved-code slots, build/upload the current source using the PlatformIO section below.** The bundled `firmware/` images predate these features.

## Prebuilt firmware — quickest route

Connect the board with a USB data cable. From this folder, install the flashing utility and run:

```sh
python -m pip install esptool==4.5.1
python tools/flash.py --port COM5 --profile full
```

Replace `COM5` with your actual port. The helper verifies the bundled images' SHA-256 checksums and writes bootloader at **0x1000**, partition table at **0x8000**, and application at **0x20000**. It does not erase NVS. If your existing firmware used a different partition map, use the initial erase procedure below before installing. `--baud 115200` can help with an unreliable USB connection.

The images have been compiled and their headers/partition layout checked; this release has not been tested on a physical E32R28T. Follow the acceptance checks below.

Use `--profile simple` for the one-LED direct-GPIO circuit. Selection is required; there is no implicit flashing default. Images are in `firmware/full/` and `firmware/simple/`. Both profiles use the same NVS layout for Favorites, saved codes and calibration.

Full: nominal 33% carrier and pre-send voltage guard on GPIO34. Invalid battery readings or voltage below ~3.40 V block sending; after a low reading, recharge to ~3.55 V to resume. Serial `BAT` reports the divided/calibrated battery reading for comparison with a meter. The guard cannot detect USB or an open LED power switch; disconnect USB for high-output TX.

Simple: nominal 50% carrier, one 150 ohm-limited emitter. It may transmit on USB power without a connected battery. Do not use simple firmware as a substitute for the full profile's voltage guard.

## PlatformIO

Install the PlatformIO extension in VS Code and open this folder. Under **Project Tasks**, select **e32r28t_full** or **e32r28t_simple** to match the wiring, then choose **Build** followed by **Upload** with the board connected by a USB data cable. Dependencies in `lib/` are bundled. PlatformIO downloads the pinned ESP32 platform/framework/compiler on the first build.

Or install Python 3 and use:

```sh
python -m pip install platformio
python -m platformio run -e e32r28t_full
python -m platformio run -e e32r28t_full --target upload --upload-port COM5
python -m platformio device monitor --baud 115200 --port COM5
```

Use `-e e32r28t_simple` for the simple build (also the PlatformIO default environment). Replace `COM5` with your actual port. Close the serial monitor before uploading. If automatic download mode fails, hold BOOT, press/release RESET, start the upload, then release BOOT once connecting starts.

This project uses a custom factory-app partition at **0x20000**, plus 64 KB NVS at **0x9000**. PlatformIO handles the partition and app offsets. Do not copy an app-only binary to the common 0x10000 offset. There is no OTA partition or filesystem partition.

On a board previously running different firmware, an initial erase may be needed if the NVS layout conflicts. An erase destroys all old firmware settings and saved codes:

```sh
python -m platformio run -e e32r28t_full --target erase --upload-port COM5
python -m platformio run -e e32r28t_full --target upload --upload-port COM5
```

Ordinary app uploads using this project's unchanged partition map preserve PULSE favorites, saved codes, and calibration. Back up important codes over USB before changing partitions or erasing flash.

## First use and calibration

Tap the calibration crosses with a stylus. After three samples, a fourth target checks the mapping. Failed validation repeats calibration. The display then opens Favorites.

To recalibrate later without erasing saved codes, open a 115200-baud terminal and send `CAL` followed by a newline. Calibration requires touching the screen; the terminal waits while that sequence completes.

Select **Listen / Emit → Eco → Power saving** for approximately 50% backlight drive and touch-wake light sleep after 20 seconds. **Default** uses full brightness and turns off only the backlight after 60 seconds. The setting survives a restart. Tap to wake before using USB commands while in Power saving.

## Physical acceptance checks

1. Verify the display and all four screen corners respond correctly after calibration.
2. Browse a known code and test it at close range before adding it to Favorites.
3. Capture one button on your original remote, emit it, save it, and power-cycle. Confirm the saved description and code persist.
4. Confirm a Favorites tap emits once, while a hold opens details without emitting.
5. Fill all 24 slots, then confirm a 25th save requests deletion instead of overwriting. Page through all eight pages; rename/delete an entry on the last page, save into the freed slot, and confirm persistence after restart. Include maximum-length RAW codes when checking storage capacity.
6. In Default, confirm the backlight goes off after 60 seconds and the first waking touch does not emit. In Power saving, confirm lower brightness, sleep after 20 seconds, and tap-to-wake without an IR transmission. Hold the waking touch, release it, then tap again to activate a button. Confirm mode selection survives a restart.
7. Test practical distance and viewing angle using your chosen emitter circuit. These depend on emitter current, optics, receiver placement, and ambient light.
8. Compare the header voltage with serial `BAT` and a meter. Verify empty/full indication at 3.00/4.20 V using an adjustable supply, five-second updates, and a fresh reading after wake. Hold a touch through an update and check that release still activates the intended button.

Never apply a 5 V receiver signal to GPIO35. Firmware does not replace the emitter current-limiting resistors or the transistor required by the higher-current build.
