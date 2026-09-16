# Third-party components

Source libraries and their license notices are included under `lib/`. IRremoteESP8266 has one documented PULSE modification: `IRsend::enableIROut()` enforces the build's `PULSE_IR_DUTY` (33 full / 50 simple), so RAW and protocol-specific calls cannot override it. Other library sources are unchanged. Git metadata is omitted. The application and build instructions are provided so the libraries can be modified and the firmware relinked.

| Component | Version / source revision | License |
| --- | --- | --- |
| [Inter](https://github.com/google/fonts/tree/main/ofl/inter) | Bundled variable TTF and generated ASCII glyph bitmaps | SIL Open Font License 1.1; `assets/fonts/OFL.txt` |
| [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI/tree/V2.5.43) | 2.5.43, `6ae4c97c5a07bd64fce8993e9cfa95d9a81e7c81` | MIT; `lib/TFT_eSPI/license.txt` |
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen/tree/v1.4) | 1.4, `d57f64c8b5f2bc5b8d10d121550806eeff7b06d9` | MIT-style notice in source headers |
| [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266/tree/v2.8.6) | 2.8.6, `2bfdf9719250471f004b720e81c14ab00a244836` | LGPL-2.1; `lib/IRremoteESP8266/LICENSE.txt`; additional per-file notices retained |
| [Flipper-IRDB](https://github.com/Lucaslhm/Flipper-IRDB) selected post-cutoff data | See `assets/catalog_report.json` | CC0-1.0, subject to the source's dated scope; `assets/FLIPPER_IRDB_LICENSE.txt` |
| [Arduino-ESP32](https://github.com/espressif/arduino-esp32/tree/2.0.17) | 2.0.17 via PlatformIO espressif32 6.10.0 | LGPL-2.1 core and component-specific notices; core/library/variant source included under `third_party/arduino-esp32`; SDK/toolchain fetched by PlatformIO |

The repository used to regenerate the catalog is larger than the bundled subset. Older database files are not licensed by the source's CC0 statement and are excluded from the importer. Unsupported/out-of-range records are explicitly listed as counts in the report. No claim is made that every contributed capture is correct or compatible with your equipment.
