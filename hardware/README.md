# Assembly sheets

Each PDF is one A3 landscape page. Zoom on screen or print at A3 for readable component values and connector labels. Both use the Inter/charcoal/orange UI style and show the E32R28T connectors from the rear.

Choose the **full build** for two battery-fed TSAL6200 emitters, each with a 33 ohm resistor, or the **simple build** for one GPIO-driven emitter with a 150 ohm resistor. Each sheet includes its parts, wiring and operating notes. Read `../HARDWARE_VALIDATION.md` for the electrical rationale and commissioning checks. Match the resistor values and firmware profile to the chosen circuit.

The source generator and original board-reference image are included. To regenerate the PDFs from the project directory:

```sh
python -m pip install reportlab fonttools
python hardware/build_sheets.py
```

Output is `output/pdf/`. The board-reference image is attributed to LCDWiki through the PDFs' source links. The Inter font and license are under `assets/fonts/`.
