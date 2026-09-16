# Hardware design and commissioning

PULSE supports two emitter circuits on the E32R28T. The **full build** uses two battery-fed Vishay TSAL6200 emitters, each with a **33 ohm, 1%, 1/2 W** resistor and a shared AO3400A driver. The **simple build** uses one GPIO-driven, 20 mA-rated emitter with a **150 ohm, 1/4 W** resistor. Both provide the same touchscreen controls, IR learning, Favorites and eight saved-code slots. Use the assembly sheet and firmware profile that match your circuit.

## Circuit specifications

| Function | Specification and rationale |
| --- | --- |
| Battery-powered LEDs | Full: separate poles of one DPST switch feed board BAT+ and the LED supply. Fit an inline 1 A fuse and use a protected 1S battery. |
| Emitter current | TSAL6200 has a 100 mA continuous rating and a 200 mA peak rating under specified 100 us / 50% conditions; observe datasheet temperature derating. The 33 ohm branches provide approximately 60-90 mA while on. Use the specified emitters and resistors; unspecified kit LEDs do not establish a suitable higher-current rating. |
| Supply capacitors | Full: 470 uF plus 100 nF across VIR/GND, close to the LEDs. Use a 10 V electrolytic and observe polarity. The capacitor supports current edges; the battery wiring must supply the packet. |
| DC-blocked gate | Full: TX -> 100 ohm -> 10 nF -> gate; gate -> 10k -> GND. BAT54 cathode K to gate, anode A to GND. Gate capacitance and clamp voltage affect the waveform. |
| Gate discharge | The 10k x 10 nF time constant is approximately 0.1 ms. An ideal 3.3 V step leaves 1.21 V after one time constant; near-zero gate voltage takes about 0.5 ms. Measure actual LED turn-off. This supplementary protection does not cover stuck PWM, a shorted MOSFET or wiring faults. |
| Receiver filter | Full: 100 ohm in the receiver 3V3 feed; 10 uF plus 100 nF from filtered VCC to GND at the receiver. Simple: direct 3V3, with optional 100 nF bypass if capture is noisy. OUT -> GPIO35. Check physical lead order. |
| GPIO34 cutoff | The board has a 100k/100k divider. Read calibrated millivolts x 2. Full blocks below ~3.40 V, resumes at ~3.55 V; invalid readings block. Samples occur before and after sending, not continuously within a mark. |
| Carrier duty | Full forces nominal 33% at the library's common carrier setup, including RAW and protocol routines. Simple uses nominal 50% for its one low-current LED. Integer timing and execution overhead mean actual duty/frequency require a scope measurement. |
| Four LEDs | Optional: four identical TSAL6200 + 33 ohm branches and 1000 uF bulk. Recheck current, inrush, fuse, switch, wiring and temperatures. |
| Range and coverage | Verify with the target equipment. Ideal identical-optics range scales approximately with the square root of radiant intensity. Beam angle, efficiency, duty, receiver and ambient light also matter. Angling LEDs outward trades central range for coverage. |

## Switch and charging

BAT+ is connected to the onboard charger. The full build uses a **DPST switch: two independent contacts, mechanically linked**, so the LED supply can be isolated from both the battery and charger:

- Protected battery + -> fuse -> pole A -> board BAT+.
- Same fused battery + -> pole B -> VIR / LED driver.
- Battery - -> board BAT- and driver/receiver ground.

OFF isolates both feeds, but USB can still run the board. Do not join the two switched outputs: that reconnects the LED supply to the charger. Allow residual bulk-capacitor charge to dissipate before wiring. ON is required to charge. Charge with IR idle; disconnect USB for high-output transmission. The ADC measures the board's BAT+ node and cannot distinguish a battery from charger voltage. Firmware does not implement USB detection, charging control or pack protection.

The simple build connects the battery directly to the board's battery socket, optionally through an SPST switch on BAT+. Its emitter draws power only from GPIO27. Opening the optional switch disconnects the battery and stops charging; USB can still power the board.

## Calculations (representative, not measured)

With Vf = 1.35 V and negligible MOSFET/wiring drop, 33 ohm gives **86 mA at 4.2 V** and **62 mA at 3.4 V**. At 86 mA, resistor dissipation is about **0.25 W while ON**, or **0.082 W** averaged over a continuous 33% carrier burst. 1/2 W parts provide margin.

Two 90 mA LEDs during a 9 us pulse cause ideal 470 uF droop I*t/C of about **3.4 mV**, excluding ESR. During a full 9 ms carrier burst without battery contribution, droop exceeds **1 V**. Good battery wiring remains essential.

Simple: (3.3 V - Vf of 1.2 to 1.5 V) / 150 ohm gives roughly **12-14 mA**. One emitter concentrates that limited drive into one optical beam; it does not increase the GPIO current rating.

## Bench acceptance

1. Disconnect USB/battery; verify polarity, shorts, fuse, diode K/A, AO3400A G=1/S=2/D=3, capacitor polarity and common ground. Do not solder to a LiPo pouch.
2. Start full-build commissioning with **150 ohm LED resistors**, then power from a current-limited supply or protected battery. Verify display/touch and capture.
3. Scope VGS and resistor voltage at 30-60 kHz RAW and 36/38/40 kHz decoded sends. Confirm clean turn-off and adequate gate drive; AO3400A's specified low RDS(on) requires at least 2.5 V. LED current = resistor voltage / actual resistance.
4. Only then fit the specified 33 ohm resistors and repeat on a freshly charged battery. Check LED, MOSFET, resistor, wiring and battery temperatures under repeated use. Lower resistance requires a separate electrical and thermal assessment.
5. With current limiting, verify a steady-high GPIO causes a transient followed by zero LED current. Verify switch-OFF isolation with USB attached, allowing for residual capacitor charge.
6. Compare serial `BAT` with a meter. If necessary set `board::BATTERY_SCALE = old_scale * meter_voltage / reported_voltage`, then rebuild. Test cutoff/release with a suitable adjustable supply, not by intentionally over-discharging a LiPo.
7. Verify range, noise, replay and saved-code persistence. Do not stare into IR LEDs or use magnifying optics; the radiation is invisible.

## Primary references

- [E32R28T schematic: divider, GPIO34, charger, connector nets](https://www.lcdwiki.com/res/E32R28T/2.8inch_ESP32-32_Display_Schematic.pdf)
- [Board documentation and physical connector locations](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)
- [TSAL6200 ratings and thermal derating](https://www.vishay.com/docs/81010/tsal6200.pdf)
- [AO3400A gate-drive specifications](https://www.aosmd.com/sites/default/files/res/datasheets/AO3400A.pdf)
- [Espressif calibrated ADC API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)

The external circuit has not been built, characterized on a scope, or simulated with vendor transistor models here. The sheet is a build plan with explicit commissioning requirements.
