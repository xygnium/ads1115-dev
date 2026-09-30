# ads1115-dev design notes

Design decisions for the ADS1115 thermistor front end, with the reasons behind each one and the evidence that would reopen it.

Background: `esp32-dev/SESSION-SUMMARY-2026-09-22.md`, section 6 (attic radiant-barrier instrumentation).

## Scope

- Precision glass-bead 10k NTC thermistors read by an ADS1115 16-bit I2C ADC.
- It must run on both the Raspberry Pi Pico (pico-sdk) and the ESP32 (ESP-IDF).
- Uses (applications live in the platform repos):
  - `esp32-dev/thermistor-cal`: the calibration rig. It verifies the boards and calibrates the probes against fixed points.
  - `esp32-dev/ambient`: a **temporary** calibrated-thermistor sidecar, used to calibrate the node's SHT45 in place and removed afterwards.
  - `pico-dev/temp-sense`: **permanent** attic probes (2+ circuits: globe, shielded air, surfaces) for radiant-barrier effectiveness.

## Code layout (proposed)

- **Portable core** (`ads1115_core/`): ADS1115 driver, ratiometric math, Steinhart-Hart, calibration coefficients. No platform includes.
- **Per-platform ports:** `ports/ads1115_pico/` and `ports/ads1115_idf/`. Each provides the same few functions: I2C write, I2C read, GPIO set, delay.
- **Directory names are component names.** ESP-IDF names a component after its directory, and names share one namespace across the consuming build. Hence the `ads1115_` prefix: `ads1115_core` and `ads1115_idf` can't be mistaken for ESP-IDF itself or clash with another library's `core` (renamed from `core/` and `ports/esp-idf/`, 2026-09-30).
- **Consumers:** applications live in the platform repos (`esp32-dev`, `pico-dev`). For now they reference this repo as a sibling checkout by relative path. A git submodule, pinned per repo, is reviewed at the end of stage 1 (`esp32-dev/thermistor-cal`, stage 1j).
- **No code is moved into this repo from existing workspaces** as part of new work. Moving code is a separate, deliberate step.

## Front end

### Ratiometric, pulsed excitation

- A GPIO drives the divider only while converting. This avoids self-heating: 0.1–0.3 °C at steady DC, and different for still-air and fan-cooled probes.
- The divider runs from the GPIO: fixed resistor, then the tap, then the thermistor (over its own CAT5 pair), then single-point ground.
- One ADS1115 channel measures the excitation voltage. Each thermistor is computed as a ratio against it, so the ADC reference error and any supply error cancel.
- Budget: **3 thermistors + 1 excitation monitor per chip.** Up to 4 chips per I2C bus (0x48–0x4B).
- Use the same PGA setting for every channel, so gain errors cancel in the ratio.

### Single-ended, not differential

**Decision:** single-ended inputs.

- The divider tap and the ground reference are both on the board. The cable only extends the thermistor, so differential mode protects nothing.
- Noise is handled elsewhere:
  - one twisted pair per probe, with its own ground leg back to a single-point ground;
  - a low data rate (8–16 SPS), whose filter rejects 50/60 Hz, or ~15 samples at 860 SPS averaged over one 60 Hz cycle.
- Differential mode halves the channels (2 per chip) and breaks the 3+1 budget.
- Lead resistance sits in series with the thermistor and calibrates out.
- Single-ended mode loses the negative half of the range, leaving ~15 bits. That's still ~0.004 °C per step at the divider midpoint.

**What would reopen it:** an in-place noise test. Put a fixed precision resistor at the far end of a full-length CAT5 run in the attic, next to the mains wiring, and log its readings. If the spread is well above the bench noise, and the low SPS rate doesn't fix it, reconsider differential mode.

### Electronics location

- Only the thermistors need to be in the attic. Analog thermistor signals are fine over 24–32 ft of CAT5 (lead resistance ≈ 0.016% of 10 kΩ).
- **Put the ADS1115, fixed resistors and MCU somewhere cooler and more stable than the attic** where practical. This reduces resistor tempco effects and aging.
- If they must go in the attic, the resistor spec below still holds.
- (Note: I2C devices, e.g. an attic SHT4x, can't do this. They need to be within 1–2 m of their MCU.)

### Fixed (divider) resistor

- **Stability matters more than nameplate tolerance.** End-to-end calibration absorbs the actual value. It can't absorb a value that moves afterwards.
- Spec: 10k, 0.1%, **≤10 ppm/°C**, thin film or better.
  - At 10 ppm/°C, a 20→50 °C swing is 0.03%, about 0.007 °C of reading error.
  - A 100 ppm/°C part would be about 0.1 °C. Avoid.
- Aging: datasheet load-life figures (e.g. 0.05–0.25% after 1000 h at 70 °C, full power) are worst case. With pulsed, near-zero dissipation at ≤50 °C, expected drift is a few hundredths of a °C per year at most. *(Estimate, not measured.)*

## Parts sourcing

Fake and relabelled parts are common on cheap breakout boards. Buy the measurement-critical parts from authorized distributors (Digi-Key, Mouser) or reputable board makers (Adafruit, SparkFun). Verify each one on arrival, whatever the source.

| part | known risk | check on arrival |
|---|---|---|
| ADS1115 | **ADS1015 (12-bit) sold as ADS1115 (16-bit)** | Read a slowly varying input. On an ADS1015, the lowest 4 bits of each result are always 0 and the step size is 16× coarser. |
| BMP390 | relabelled or older Bosch parts (BMP388/BMP280) | Read the chip ID: BMP390/BMP388 register 0x00 → 0x60 / 0x50. BMP280 uses register 0xD0 → 0x58. Also cross-check pressure against the airport altimeter setting (see Calibration). |
| precision resistors | off-spec tempco or tolerance | Buy from distributors only. Tempco can't be verified cheaply at home, and the check-standard resistor will reveal drift over time. |
| NTC thermistors | unknown curve or tolerance | Buy from a named maker (e.g. TE, Amphenol, Littelfuse) with a datasheet R-T table. Calibration covers the rest, and the datasheet cross-check flags an odd curve. |

Calibration absorbs a part's value and gain errors. It can't recover missing resolution (a fake ADS1115), and it can't prevent a bad tempco from drifting after calibration.

### ADS1115 boards in use: Lonely Binary

Third-party Amazon breakout, not from an authorized TI distributor. No published schematic found. Check **every board** on arrival and label each one once it passes.

1. **Visual:** read the chip marking with a loupe. The ADS1115 package (VSSOP-10) is marked **BOGI**, the ADS1015 **BRPI**. Relabelled parts can carry the right marking, so the resolution test still decides.
2. **Resolution (the decisive test):** at 3.3 V, PGA ±4.096 V (so a 0–3.3 V pot sweep stays in range), read a slowly varying input (e.g. a potentiometer, or a thermistor warmed by a finger) for a few hundred samples. A genuine ADS1115 produces odd and even codes with steps of 1. An ADS1015 has the bottom 4 bits always 0, so every code is a multiple of 16.
3. **Noise floor:** short one input to GND and log ~100 readings at 8 SPS. Expect a spread of a few LSB at most. A much wider spread points to a bad board, a bad part, or a wiring problem.
4. **Pull-ups:** with the board unpowered, measure SDA→VDD and SCL→VDD. Record the value (typically 10k). With N boards on one bus, the effective pull-up is value/N. Keep the total ≥ ~2k so the MCU can still pull the lines low at 3.3 V. Remove extra pull-ups if needed.
5. **ADDR default:** measure ADDR→GND. Expect a pull-down, giving 0x48. Confirm with an I2C scan. Then strap each board to its address (VDD → 0x49, SDA → 0x4A, SCL → 0x4B), rescan, and check the TI datasheet's timing notes before relying on SDA/SCL strapping.
6. **Power:** run at 3.3 V from the MCU rail, not 5 V, so the I2C lines stay at MCU logic level.
7. **Input filtering:** check for any series resistors or capacitors on A0–A3. Assume none, and add the 1 kΩ + 100 nF RC at each used input on your own wiring.

## Calibration

### Principle

- Calibrate **end to end**: probe, cable, divider, ADC. This absorbs the fixed resistor's actual value, ADC gain, mux and lead resistance in one step.
- Anchor to physical fixed points, not to instruments. Every measuring tool carries its own error down the chain. A fixed point does not.
- Three points let the full **Steinhart-Hart** equation be fitted. With only two points widely spaced (0 and ~99 °C), the curve can sag in the 20–50 °C range that matters most.

### The three points

| point | temperature | reagent / setup | notes |
|---|---|---|---|
| ice point | 0.00 °C | crushed ice from distilled water, packed slush | no instrument needed |
| Glauber's salt transition | ~32.38 °C | Na₂SO₄ ⋅ 10H₂O ⇌ Na₂SO₄ + saturated solution | mid-range; no instrument needed |
| boiling / steam point | ~99.97 °C at 1013.25 hPa | boiling distilled water, corrected for **station pressure** | needs a barometric reading |

### Glauber's salt preparation (from `thermistor-calibration.txt`)

- Start from **ACS reagent grade anhydrous sodium sulfate** (≥99%, with a certificate of analysis for the lot).
- Hydration ratio: the decahydrate is 55.9% water by weight. A working mix is **70 g anhydrous + 100 g distilled water**, which leaves a little free liquid on purpose.
- Dissolve at 40–45 °C, stirring, then cool while stirring to crystallize a thick slush.
- The mix is reusable: seal it in a jar, then re-melt and re-crystallize for the next calibration.

### Concerns and advice

**Glauber's salt**
- **Take the reading while warming, not cooling.** Sodium sulfate decahydrate is known for **supercooling**: it can stay liquid below 32.38 °C and then jump up when it finally crystallizes. `thermistor-calibration.txt` takes the reading on the cooling side. The melting plateau, warming slowly through the transition, is usually the trustworthy one.
- **It melts incongruently.** Above the transition, anhydrous crystals form and settle to the bottom. Stir gently and continuously so all three phases (decahydrate, anhydrous, saturated solution) stay around the probe.
- **The exact ratio isn't critical.** The transition is a three-phase equilibrium, so the water ratio sets how long the plateau lasts, not its temperature.
- **The accepted value** is quoted as ~32.38 °C (32.374–32.384 in various sources). Pin down one citable value before relying on the last hundredth.
- **Insulate the vessel** (a foam box or a double-walled cup) and warm it slowly, e.g. in a controlled water bath. A long, flat plateau matters more than speed.

**Boiling / steam point**
- **Use station pressure, not sea-level pressure.** Weather sites and airport reports give pressure adjusted to sea level, which can be wrong by tens of hPa at altitude. The effect is about **0.028 °C per hPa**. Use a local barometer, or a station report that gives unadjusted pressure.
- On the ITS-90 temperature scale, water boils at **99.974 °C at 1013.25 hPa, not 100.000**.
- **Convert pressure to boiling point with IAPWS-IF97** (the region-4 saturation-temperature equation). Checked linear fit, good within ±30 hPa of standard pressure:

  `T_boil (°C) = 99.974 + 0.0279 × (P_station_hPa − 1013.25)`

  Checks: 1000 hPa → 99.606 °C, 990 hPa → 99.325 °C (IF97).
  - Common shortcut formulas give wrong results. `100.0 + 0.0343 × ΔP` uses the wrong baseline and a slope 23% too high, and at 990 hPa it is about 0.12 °C low. Verify any formula against IF97 before using it.
- **Measure station pressure on the bench** with an absolute barometer (e.g. BMP390, ~±0.5 hPa absolute ≈ ±0.014 °C) at the moment of the reading. This removes any error from the bench elevation (10 m ≈ 1.2 hPa ≈ 0.03 °C) and from reducing the pressure from a distant weather station. See *Parts sourcing* for verifying the barometer is genuine.
- **Cross-check the barometer** against a nearby airport's altimeter setting, reduced to your bench elevation with the standard-atmosphere formula: `P_station = P_alt × (1 − 0.0065·h/288.15)^5.255`, with h in metres. This is a sanity check, not a calibration. Agreement within ~1–2 hPa is expected.
- Use **distilled water**. Dissolved solids raise the boiling point.
- Keep the vessel **open**. A lid or narrow neck changes the pressure over the water.
- **Prefer the steam point:** put the probe in the vapor just above a rolling boil, not in the liquid. The liquid can superheat locally near the heated bottom, and the vapor sits at the equilibrium temperature. This is the classic hypsometer method.
- The probe must not touch the pot. Insulation on anything that reaches the heat must be rated for it: CAT5 and PVC hookup wire (60–75 °C) are **not**. Use PTFE- or silicone-insulated leads for this step.

**Ice point**
- Use ice made from **distilled water**, finely crushed and packed. Top up with distilled water to fill the gaps, then drain the excess so it's a slush, not floating ice. Floating ice with water underneath can sit above 0 °C, since water is densest at 4 °C.
- Immerse the probe deep enough to avoid stem conduction, several cm of lead below the surface.
- Wait for a stable reading. The ice point is also the **annual re-check**: minutes of work that catch drift in the resistor, ADC or probe.

**All baths**
- **Waterproof the leads, not just the bead.** The glass bead is hermetic, but bare leads or solder joints in water let current leak through the water around the thermistor, which reads as a resistance error. Seal the joints with adhesive-lined heat shrink, or keep them above the surface.
- **Calibrate all probes together** in each bath, bundled, so they share the same temperature.
- **Cross-check the fit** against the thermistor manufacturer's R-T table, using the Steinhart-Hart C coefficient from the datasheet. A large disagreement points to a bad plateau or setup, not to the datasheet.
- **Check standards:** keep one precision resistor as a permanent stand-in for a thermistor. It needs to be stable, not precisely known. Read it periodically: if it moves, the electronics drifted, not the probes.

## Uncertainties

1. **How flat and repeatable the Glauber's salt plateau is in a DIY setup.** The equilibrium is well established; the practical plateau quality is not yet known.
2. **Supercooling behavior** of this particular mix. Confirm on the first run which direction gives the flatter plateau.
3. **Estimated accuracy** after 3-point calibration. Expected to be a few hundredths of a °C near the fixed points, but not yet demonstrated.
4. **ADS1115 integral nonlinearity**, recalled as about ±1 LSB. Check against the datasheet.
5. **BMP390 absolute accuracy (~±0.5 hPa) and chip ID values** are quoted from memory. Check against the Bosch datasheet.
6. **ADS1115/ADS1015 package markings (BOGI/BRPI)** are quoted from memory. Check against TI's package-marking table.
7. **Lonely Binary board details** (pull-up values, ADDR pull-down, input components) are assumed from typical boards. Arrival checks 4–7 confirm them.
