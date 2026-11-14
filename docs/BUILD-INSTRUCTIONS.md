# Build instructions: SERGE, a Serge Modular system for VCV Rack

**Audience:** an Opus model implementing this plugin from scratch.
**Owner:** Haer. Fluent in VCV Rack, does not read C++. Build the whole plugin in one go, test it yourself, hand over the finished result. No milestone check-ins.

**What it is:** a VCV Rack 2 plugin, brand and slug `Serge`, containing 19 modules that reproduce the classic 1970s/80s Serge Modular Music Systems circuits (Serge Tcherepnin's designs, as later rebuilt by CGS/Ken Stone and Random\*Source).

**Haer's decisions (2026-11-14), which override anything else:**

| Question | Answer |
|---|---|
| Faithful or inspired-by | **As faithful as possible to the original circuits. No modern extras.** |
| Name | "Serge" is fine. This is a personal plugin and will not go to the VCV Library. |
| Look | Paperface-style panels (colour, line work, coloured banana jacks), but with **accurate text labels on every control and jack**, not symbols (changed by Haer the same evening). |
| Modules | Core + voice (no Wilson Analog Delay) + control (no TKB). |
| Coverage goal | Should cover the Random\*Source "La Bestia" and Loudest Warning "Sing To Me" panels (see §2.2). |

---

## 0. Ground rules

1. **"No modern extras" means:** no polyphony (every port is mono; read channel 0 only, output 1 channel), no added inputs, outputs, knobs, modes, displays or context-menu options beyond what is listed here. No presets beyond Rack's own. If a control is not in this file, it does not exist.
2. **Internal quality measures are not extras.** Oversampling, band-limited steps and zero-delay filters are how a digital model stays faithful to an analog circuit. They have no user-facing controls.
3. **Model the circuit, not the idea.** Each module section gives a behavioural model with constants. Where a real schematic is reachable (§12 sources), check the model against it and prefer the schematic. Record every such change in `docs/DECISIONS.md`.
4. **Self-patching must work.** Serge patches route a module's output into its own input (DSG END to TRIG, SSG CYCLE to IN, VCFQ BP to IN). Rack adds one sample of cable delay, which is fine. Nothing may blow up, produce NaN or stick. Every state variable is clamped to the op-amp rails (§3.2) and every `process()` ends with an `isfinite` guard that resets state to 0.
5. **DSP separate from Rack.** All models live in `src/dsp/` as plain C++17 with no Rack includes, so they can be unit tested on the host. `src/modules/*.cpp` are thin wrappers.
6. **No allocation, locks, file I/O or logging in `process()`.**
7. **Every param has a tooltip and unit, every port a name** (`configParam`, `configInput`, `configOutput`). The tooltip name is the same wording as the printed panel label (§4.1), spelled out in full where the label is abbreviated.
8. **Ambiguity:** pick the reading closest to the original hardware, write it in `docs/DECISIONS.md`, list it in the hand-over.
9. **CPU:** each module under 1% of one core at 48 kHz, RESEQ under 2%.
10. Licence GPL-3.0-or-later. Do not copy code from other plugins unless compatible, and credit it in `docs/CREDITS.md`.

---

## 1. Setup

- **VCV Rack 2.x**, newest 2.x Rack SDK from https://vcvrack.com/downloads/. Read https://vcvrack.com/manual/PluginDevelopmentTutorial, https://vcvrack.com/manual/Building, https://vcvrack.com/manual/Panel and https://vcvrack.com/manual/DSP first.
- `export RACK_DIR=/path/to/Rack-SDK`, then `make`, `make dist`, `make install`.
- Unit tests with **doctest** (single header) in `tests/`; `make test` builds `tests/*.cpp` with `src/dsp/*.cpp` as a host executable and runs it.
- Add `.github/workflows/build.yml` that builds Windows x64, macOS x64 + arm64 and Linux x64 with the official `rack-plugin-toolchain` Docker image (or the community `build-plugin` action based on it), runs `make test` on Linux, and uploads the `.vcvplugin` files as artifacts on every push, and as release assets on tags `v*`. Haer installs by dropping the `.vcvplugin` into Rack's `plugins-<os>-<cpu>` user folder; put those steps in `README.md`.

```
plugin.json   Makefile   README.md   LICENSE
docs/   DECISIONS.md  CREDITS.md  MANUAL.md   (and this file, as docs/BUILD-INSTRUCTIONS.md)
res/    <Module>.svg ×19,  components/ (jack, knob, switch, LED SVGs),  fonts if needed
src/
  plugin.hpp  plugin.cpp
  dsp/        Common.hpp  Slew.hpp  Svf.hpp  Oversample.hpp  Blep.hpp  Noise.hpp
              Dsg.hpp Ssg.hpp Nto.hpp Vcm.hpp Vcfq.hpp Vcfs.hpp ResEq.hpp Tws.hpp Ring.hpp ... (.cpp as needed)
  modules/    one .cpp per module
  widgets/    SergeComponents.hpp   (ports, knobs, switches, LEDs)
tests/
```

`plugin.json`: slug `Serge`, name `Serge`, brand `Serge`, version `2.0.0`, licence `GPL-3.0-or-later`. Module slugs as in §2.1. Tags per module from Rack's tag list.

---

## 2. The module set

### 2.1 Modules

| # | Slug | Panel name | Original | Width |
|---|---|---|---|---|
| 1 | `DSG` | Dual Universal Slope Generator | DSG / DUSG | 12 HP |
| 2 | `SSG` | Smooth & Stepped Generator | SSG | 10 HP |
| 3 | `NTO` | New Timbral Oscillator | NTO | 16 HP |
| 4 | `VCM` | Wave Multipliers | VCM | 12 HP |
| 5 | `VCFQ` | Variable Q VCF | VCFQ | 10 HP |
| 6 | `VCFS` | Variable Slope VCF | VCFS | 10 HP |
| 7 | `RESEQ` | Resonant Equalizer | EQ | 20 HP |
| 8 | `TWS` | Triple Waveshaper | TWS | 8 HP |
| 9 | `RING` | Ring Modulator | RING | 8 HP |
| 10 | `NOI` | Noise Source | NOI | 6 HP |
| 11 | `ASR` | Analog Shift Register | ASR | 6 HP |
| 12 | `COM` | Dual Comparator | COM | 8 HP |
| 13 | `STR` | Dual Schmitt Trigger | STR | 6 HP |
| 14 | `PRC` | Dual Processor | PRC | 10 HP |
| 15 | `XFAD` | Cross-Fader | XFAD | 8 HP |
| 16 | `VCA2` | Dual VCA | 2VCA | 8 HP |
| 17 | `MIX` | Dual Audio Mixer | MIX | 10 HP |
| 18 | `DCSM` | Dual Channel Stereo Mixer | DCSM | 12 HP |
| 19 | `PT` | Peak & Trough | Peak/Trough | 4 HP |

Widths are a guide. Widen in 2 HP steps if controls do not fit with the clearances in §4.4, and note it.

Modules 1 to 15 are Haer's chosen set, with the grouped panels of the proposal split into the separate modules Serge sold (Triple Waveshaper and Ring; Noise and ASR; Comparator, Schmitt, Processor, Cross-Fader). Modules 16 to 19 were added to cover the two reference panels.

### 2.2 Coverage of the reference panels

| Panel block | Covered by |
|---|---|
| **La Bestia II:** NTO | `NTO` |
| Audio Mixer | `MIX` |
| Wave Multipliers | `VCM` |
| SSG + Noise Source | `SSG`, `NOI` |
| DSG mk2 | `DSG` (classic version, not the mk2 extras) |
| Variable Slope VCF | `VCFS` |
| Variable Q VCF | `VCFQ` |
| Stereo Mixer | `DCSM` |
| **Sing To Me:** Triple Wave Shaper | `TWS` |
| CV / Audio Processor | `PRC` |
| R6 Ring Mod | `RING` |
| Wave Multipliers, SSG, Noise, VCFQ | `VCM`, `SSG`, `NOI`, `VCFQ` |
| DTG (Dual Transient Generator) | `DSG` (the catalog says the DTG is a DSG with fewer panel features) |
| Dual VCA | `VCA2` |
| DCSM | `DCSM` |
| Bonus mixer / peak / trough | `MIX`, `PT` |
| 2 × Buchla 258 VCO | **Not built.** Not a Serge circuit. `NTO` takes its place. |

**Not covered, deliberately:** La Bestia III's new-generation blocks (GTO, GTS, New Gate / Dual Sine oscillators, Mixer+ Fuzz). They are 2020s redesigns, not the original circuits. Their ancestors (`SSG`, `DSG`) are here.

---

## 3. System conventions

### 3.1 Signal standards (Serge native, not Rack's ±5 V / 10 V)

The plugin uses Serge levels at every jack. This keeps every threshold and nonlinearity where the circuit has it.

| Kind | Level | Jack colour |
|---|---|---|
| AC / bipolar (audio) | nominal ±2.5 V (5 Vpp) | **black** |
| DC / unipolar (control) | 0 to +5 V | **blue** |
| Pulse (gate, trigger, logic) | 0 V or +5 V | **red** |
| Rail-swinging outputs (SSG CYCLE, HOT coupler) | about ±10.5 V | **yellow** (Serge's own recommendation) |

- Pitch inputs are 1 V/octave.
- A pulse input fires when its voltage rises through **+2.5 V**; it re-arms when it falls below +2.0 V. Any signal can drive a pulse input.
- Inputs do not care what kind of signal they get. Nothing is AC-coupled unless stated.
- Unpatched inputs read 0 V.
- Put a short note in `docs/MANUAL.md`: Rack's own modules use 10 Vpp audio and 10 V gates, so Serge outputs look half-size to them and Rack oscillators run hot into Serge shapers. That is expected.

### 3.2 Shared DSP (`src/dsp/Common.hpp` and friends)

- `RAIL = 10.5f` (op-amp output limit on ±12 V supplies). `railClip(x)`: linear to ±9.5, soft knee to ±10.5 (`x` beyond 9.5 mapped by `9.5 + tanh(x − 9.5)`).
- `SchmittEdge`: rising-edge detector with the 2.5 / 2.0 V thresholds above.
- **Slew core** (`Slew.hpp`), used by DSG and SSG. The analog circuit is an error amplifier driving current sources into a capacitor: a high-gain follower whose speed is limited separately upward and downward.
  ```
  e = target − y
  want = G * e                          // G = 50000 1/s (follower bandwidth about 8 kHz)
  rate = clamp(want, −fallRate, +riseRate)      // V/s
  if (|want| inside the limits)  y += e * (1 − exp(−G*dt));   // settling
  else                           y += rate*dt, never stepping past target
  y = clamp(y, −RAIL, RAIL)
  ```
- **Oversampling** (`Oversample.hpp`): factor 4 at sample rates up to 50 kHz, factor 2 up to 100 kHz, otherwise 1. Polyphase half-band FIR (or `rack::dsp::Upsampler/Decimator` wrapped so the dsp code stays Rack-free; a small copy of a half-band design is fine). Used by NTO, VCM, TWS, RING, VCFQ, VCFS, RESEQ, VCA2 and the HI mode of anything that clips.
- **SVF** (`Svf.hpp`): zero-delay-feedback (TPT, Zavalishin) state-variable filter with `g = tan(π f / fs_os)`, damping `k = 1/Q`, outputs LP, BP, HP. Nonlinear option: the two integrator inputs pass through `otaSat(v) = 6·tanh(v/6)` (CA3080 input stage, generous because Serge linearises with input attenuation); solve the feedback with one Newton step per sample or the standard "tanh on the state estimate" approximation.
- **Exponential converter:** `f = f0 · 2^(volts)`, clamp to `[0.01 Hz, 0.45·fs_os]`.
- **Noise** (`Noise.hpp`): xoshiro128+ per module, seeded from `random::u64()` at construction; Gaussian by sum of 4 uniforms.
- Knob smoothing: one-pole 5 ms on every continuous parameter that scales audio. Switches are not smoothed (they clicked on the hardware too).

---

## 4. Panels: paperface style with text labels

The original "paperface" Serge panels (1973 to about 1976) were printed paper graphics under clear laminate on aluminium: off-white ground, black line art and coloured banana jacks. This plugin keeps that look but **labels everything in words**, using the names Serge printed on the later labelled panels and in the catalog. Haer asked for accurate labels in place of the paperface symbols.

### 4.1 Look
- Panel ground: warm off-white `#ece6d6` with a very faint paper grain (a low-contrast noise texture as an embedded pattern or a few hundred tiny 3% opacity specks; keep SVG size reasonable). 0.4 mm black border inset 1 mm. Four black "screw" circles are fine; use Rack's `ScrewBlack`.
- Ink: `#161616`. Line weight 0.35 mm for graphics, 0.6 mm for section dividers.
- **Function blocks:** each section of a module (for example each half of the DSG, each of the three Wave Multipliers) sits in its own outlined region. Controls and jacks that belong together are joined by thin lines; a CV input is joined to the attenuator that scales it.
- **Labels are text, and they are exact.**
  - **Every knob, switch position, jack and LED has a printed text label.** No control is identified by a symbol alone. Do not draw waveform pictures, ramp icons, filter-curve icons, arrows-as-labels or other pictograms in place of words.
  - **The label text is the Name given in that module's table in §5 to §8, verbatim, in capitals**: `RISE`, `FALL`, `VC`, `IN`, `TRIG`, `VC IN`, `1V/OCT`, `OUT`, `END`, `RATE`, `HOLD`, `SAMPLE`, `CYCLE`, `COUPLER`, `HOT COUPLER`, `FREQ`, `FINE`, `PORT`, `SAW`, `TRI`, `SINE`, `VAR`, `LOW`, `BAND`, `HIGH`, `NOTCH`, `HI` / `LO`, and so on. Those names are the ones used on Serge panels and in the Serge catalog; do not rename, translate or "improve" them.
  - Where a table gives a description instead of a name, use these: VCM top knob `VC`, middle knob `FOLD`, bottom knob `VC`; VCM bottom outputs `OUT` and `SQUARED`; DSG switch positions `RISE`, `BOTH`, `FALL`; RESEQ band knobs labelled with their frequency exactly as `29`, `61`, `115`, `218`, `411`, `777`, `1.5K`, `2.8K`, `5.2K`, `11K`; RESEQ outputs `OUT`, `COMB ↑`, `COMB ↓` (the arrows are part of the original label and are the one permitted glyph); COM inputs `+` and `−`; attenuverter knobs carry the name of what they scale (`VC`, `VC F`, `VC Q`, `VC WAVE` ...), with small `−` and `+` at the ends of the knob scale.
  - Sections of a multi-section module get a small section title: DSG `A` and `B`; SSG `SMOOTH` and `STEPPED`; VCM `1`, `2`, `3`; TWS `1`, `2`, `3`; COM, PRC, VCA2, MIX `A` and `B`; DCSM `1` and `2`; STR `A` and `B`.
  - The module title at the top is the **full name** from the §2.1 "Panel name" column in capitals (wrap to two lines if needed), e.g. `DUAL UNIVERSAL SLOPE GENERATOR`, `SMOOTH & STEPPED GENERATOR`, `WAVE MULTIPLIERS`, `VARIABLE Q VCF`.
  - Placement: a knob's label is centred **above** it; a jack's label is centred **below** it (above only if space forces it, and then consistently within the block). Labels never overlap a component, a line or each other, and are never rotated.
  - Type: a plain, upright sans or drafting-stencil face (an OFL font such as "Share Tech Mono", "Jost" or "DIN"-style), all capitals, 2.2 mm cap height for labels, 1.8 mm minimum where space is tight, 3 mm for the module title. **Convert all text to paths** (Rack does not render SVG text; `inkscape --export-text-to-path`, or build glyph paths in a script).
  - **The check:** the set of printed labels on a panel must equal the set of names in that module's table. `scripts/make_panels.py` must build both from one shared table and fail if a control has no label.
- Inputs and outputs are told apart by wording (`IN`, `OUT`, and so on) and by position (§4.4). A thin outline box around each block's group of outputs is allowed.
- No brand logo. The word `SERGE` in small capitals at the bottom of each panel.
- Knob scales: a simple arc with a dot at each end and, for bipolar knobs, a tick at 12 o'clock.

### 4.2 Components (`res/components/`, `src/widgets/SergeComponents.hpp`)
- **Banana jack ports**, one SVG per colour: `JackBlack`, `JackBlue` `#1f4fa8`, `JackRed` `#c0281c`, `JackYellow` `#e0b01a`. A banana socket seen from the front: a coloured plastic collar 8.4 mm across with a slightly lighter bevel ring and a 4 mm dark hole with a thin brass ring. Subclass `app::SvgPort`. The colour follows the signal type in §3.1 and each module's tables. Inputs and outputs use the same jack (as on the hardware); the printed labels tell them apart.
- **Knob:** `SergeKnob`, the black fluted, skirted knob of the period (Davies 1900 type): 12 mm body, black with a white indicator line, subtle top highlight. Subclass `app::SvgKnob`, range −135° to +135°. A smaller `SergeKnobSmall` (9 mm) for attenuators if space demands.
- **Switches:** `SergeToggle2`, `SergeToggle3`: chrome bat-handle toggles, 2 or 3 frame SVGs (`app::SvgSwitch`).
- **LED:** red 3 mm (`MediumLight<RedLight>` is acceptable).
- Cables: leave Rack's cable colours alone.

### 4.3 Making the SVGs
Generate each panel with a script (`scripts/make_panels.py`, plain Python writing SVG) so positions come from the same table the C++ uses. Panel height 128.5 mm, width `HP × 5.08` mm. Include a hidden `components` layer with Rack's colour-coded placeholders if you use `helper.py`; otherwise place widgets in code with `mm2px(Vec(x, y))` from the shared table. Render every panel to PNG in CI (or locally with Playwright/Inkscape) and look at each one before hand-over: no overlaps, no clipped text, every control and jack labelled with the right name.

### 4.4 Layout rules
- Knobs in the upper part of each function block, jacks below them, outputs toward the right or top edge of the block, as on Serge panels.
- Minimum centre spacing: jack to jack 10.5 mm, knob to knob 15 mm, knob to jack 13 mm.
- A CV input sits directly under the attenuator knob that scales it, joined by a line.
- Leave room for the labels: at least 4 mm of clear panel above each knob and below each jack. Widen the module rather than shrink or drop a label.

---

## 5. Core modules

Notation: **K** = knob, **S** = switch, **I** = input, **O** = output, **L** = light. Colours per §3.1. "Attenuator" = 0 to 1. "Attenuverter" = −1 to +1, centre 0.

### 5.1 DSG, Dual Universal Slope Generator

Two identical, independent sections. Each is a unity-gain voltage follower whose upward and downward slew rates are separately voltage controlled, plus a trigger latch that turns it into an envelope. Slopes are **linear** (constant V/s); curved shapes come only from patching OUT back into VC IN.

**Per section:**

| | Name | Details |
|---|---|---|
| K | RISE | rise rate. Default 0.5 |
| K | FALL | fall rate. Default 0.5 |
| K | VC | attenuverter for VC IN. Default 0 |
| S | RISE / BOTH / FALL | 3-position: where VC IN is applied. Default BOTH |
| I | IN | black. Signal to follow |
| I | TRIG | red. Starts one rise-fall cycle |
| I | VC IN | blue. Through the VC knob, 1 oct/V at full |
| I | 1V/OCT | blue. Calibrated, applies to both rise and fall |
| O | OUT | blue |
| O | END | red. 0 / +5 V |
| L | level | brightness = OUT/5 |

**Model** (per sample; uses the slew core of §3.2):

```
rateKnob(k) = 0.25 * 2^(17.6*k)                    // V/s. k=0: 20 s per 5 V. k=1: 100 µs per 5 V
vc   = VCknob * VC_IN
rise = rateKnob(RISE) * 2^( oct + (switch != FALL ? vc : 0) )      // oct = 1V/OCT input
fall = rateKnob(FALL) * 2^( oct + (switch != RISE ? vc : 0) )
clamp both to [0.005, 150000] V/s

u = IN
target = latch ? max(u, 6.0) : u                   // the latch injects a level above the peak detector
y = slew(y, target, rise, fall)
if (latch && y >= 5.0) latch = false               // peak detector, calibrated for a 0 to +5 V envelope

falling: set when y − target > 0.020, cleared when y − target < 0.005     // comparator with hysteresis
END = falling ? 0 : 5                              // high while rising and at rest, low during the fall
busy = latch || falling
on TRIG rising edge: if (!busy) latch = true       // triggers during a cycle are ignored
OUT = y
```

**Behaviours this must reproduce** (write a unit test for each):
1. Trigger: one envelope 0 → +5 V → 0. A second trigger before the end does nothing.
2. Gate into IN: rises at RISE to the gate level, holds, falls at FALL. A gate arriving during the fall rises again from where it is.
3. END patched to TRIG: free-running. Both knobs fully clockwise gives a triangle of about 5 kHz (4 to 5 kHz accepted); turning one down gives a sawtooth at lower frequency. END is the pulse wave, low for the fall time.
4. Sub-harmonic division: clock into TRIG with rise+fall slightly longer than one clock period gives a cycle every second clock, and so on.
5. Pulse delay: END rises again rise+fall after the trigger.
6. Portamento: stepped voltages into IN glide up at RISE and down at FALL; both knobs fully clockwise passes the signal essentially unchanged.
7. Envelope follower: audio into IN, RISE fully clockwise, FALL moderate, follows positive peaks. FALL fully clockwise and RISE moderate follows negative peaks.
8. 1V/OCT doubles the cycling frequency per volt, within 3% over 4 octaves below 1 kHz.
9. OUT into VC IN with VC positive bends the slopes one way, negative the other (log/exp shapes).
10. A positive level at IN keeps the output up regardless of triggers.

Anti-aliasing: none needed beyond the slew core; when cycling at audio rate compute the turn-around point inside the sample (do not let `y` overshoot 5.0; carry the remainder of the sample into the fall) so pitch is exact.

### 5.2 SSG, Smooth & Stepped Generator

Two slew sections sharing one design, plus a comparator ("coupler"). The **Smooth** section is a slew limiter with a HOLD input (track-and-hold). The **Stepped** section is the same slew limiter, but it only tracks during a **1.0 ms one-shot pulse** fired by each rising edge at SAMPLE; that is what makes it a sample-and-hold whose step size is limited by the rate.

| Section | | Name | Details |
|---|---|---|---|
| Smooth | K | RATE | default 0.5 |
| | K | VC | attenuator (0 to 1, no inversion) for VC IN |
| | I | IN | black |
| | I | VC IN | blue, 1 oct/V at full |
| | I | HOLD | red. High = output frozen |
| | O | CYCLE | yellow, ±10.5 V |
| | O | OUT | blue |
| Stepped | K | RATE, VC | as above |
| | I | IN, VC IN | as above |
| | I | SAMPLE | red |
| | O | CYCLE | yellow |
| | O | OUT | blue |
| Shared | O | COUPLER | red, 0 / +5 V |
| | O | HOT COUPLER | yellow, ±10.5 V |
| | L | one LED per section | bipolar: brightness = |OUT|/5 |

**Model:**

```
rate = 0.5 * 2^(12*RATE) * 2^(VC * VC_IN)        // V/s. Knob alone: 0.5 to 2048 V/s. Clamp [0.01, 100000]

Smooth:   tracking = (HOLD < 2.5)                // plain level test, no edge
Stepped:  on SAMPLE rising edge: timer = 1.0 ms;  tracking = (timer > 0);  timer −= dt
          // rising edges only. Sub-sample accuracy: track for exactly the part of the sample inside the pulse.

if (tracking) y = slew(y, IN, rate, rate)        // symmetric slew core, §3.2
else          y unchanged                        // hold. No droop.

CYCLE: Schmitt on y.  y > +2.5 → CYCLE = −10.5;   y < −2.5 → CYCLE = +10.5;   otherwise keep. Power-up state +10.5.
COUPLER     = (y_stepped > y_smooth) ? 5 : 0
HOT COUPLER = (y_stepped > y_smooth) ? +10.5 : −10.5
```

**Must reproduce:**
1. Smooth CYCLE → Smooth IN: triangle ±2.5 V; RATE fully clockwise about 200 Hz, fully anticlockwise sub-audio (tens of seconds). VC IN raises the rate.
2. HOLD high freezes the Smooth output; low resumes tracking (track-and-hold).
3. Stepped with a clock at SAMPLE and Stepped CYCLE → Stepped IN: an up-and-down staircase. At full RATE each step is about 2 V; lower RATE gives more, smaller steps.
4. Noise into Stepped IN, clock into SAMPLE, low RATE: random steps that wander gradually. High RATE plus positive VC: a normal sample-and-hold.
5. Smooth CYCLE → Stepped SAMPLE and Stepped CYCLE → Stepped IN works as the kit's test patch.
6. COUPLER flips whenever the two outputs cross.

### 5.3 NTO, New Timbral Oscillator

Sawtooth-core VCO with triangle and sine derived by shaping, a voltage-controlled variable waveform, linear FM with a built-in depth VCA, exponential FM, sync, and one pitch input with its own portamento.

| | Name | Details |
|---|---|---|
| K | FREQ | coarse, 16 Hz to 16 kHz exponential (10 octaves). Default 261.6 Hz |
| K | FINE | ±0.5 octave |
| K | PORT | portamento time for the PORT pitch input, 0 to 2 s time constant (cubic taper) |
| K | VC FREQ | attenuverter for FREQ CV, 1 oct/V at full |
| K | FM | linear FM depth, 0 to 1 |
| K | WAVE | variable waveform, 0 to 1 |
| K | VC WAVE | attenuverter for WAVE CV |
| I | 1V/OCT | blue, calibrated |
| I | 1V/OCT PORT | blue, calibrated, through the portamento lag |
| I | FREQ CV | blue, through VC FREQ (exponential) |
| I | LIN FM | black, modulator |
| I | FM VC | blue, depth control voltage |
| I | WAVE CV | blue |
| I | SYNC | red |
| O | SAW | blue, 0 to +5 V rising |
| O | TRI | blue, 0 to +5 V |
| O | SINE | black, ±2.5 V |
| O | VAR | black, ±2.5 V |

**Model** (run at the oversampled rate):

```
port:  p += (V_port − p) * (1 − exp(−dt/τ)),  τ = 2·PORT³ s   (τ = 0 → p = V_port)
fexp = FREQknobHz * 2^( FINE + V_1voct + p + VC_FREQ*V_freqcv )
depth = clamp(FM + V_fmvc/5, 0, 1)                     // knob and CV sum; the VCA is linear
f = fexp * max(0, 1 + depth * 2.0 * V_linfm/2.5)       // linear FM, index up to 2 at ±2.5 V. Stalls at 0, no through-zero.
f = clamp(f, 0, 0.45*fs_os)

phase += f*dt;  wrap at 1 with polyBLEP on the saw; polyBLAMP on the triangle corners
SYNC rising edge (sub-sample position): reset phase to 0 only if phase >= 0.70      // soft sync by threshold pulse
saw = phase                                 // 0..1
tri = 1 − |2*saw − 1|                       // 0..1, peak at mid-cycle
x   = 2*tri − 1
sin = sin(π/2 * x) + 0.02*sin(3π/2 * x)     // diode shaper: about 2% third harmonic
SAW = 5*saw;  TRI = 5*tri;  SINE = 2.5*sin
w = clamp(WAVE + VC_WAVE*V_wavecv/5, 0, 1)
VAR = 2.5 * ( (1 − w)*sin + w*(2*saw − 1) )
```

Tests: 1 V/oct tracking error under 1 cent from 16 Hz to 8 kHz; alias components below −70 dB for a 2 kHz saw at 48 kHz; sync locks to a master at 1×, 2× and 0.5× its frequency; linear FM at depth 1 with an equal-frequency modulator keeps the average pitch within 5 cents.

### 5.4 VCM, Wave Multipliers

Three independent waveshapers. No connections between them.

**Top (VCA / squarer)**

| | Name | Details |
|---|---|---|
| K | level | 0 to 1 |
| S | HI / LO | default LO |
| I | IN | black |
| I | VC | blue |
| O | OUT | black |

```
g = clamp(knob + VC/5, 0, 1)
LO:  OUT = g * 2.5 * tanh(0.4 * IN/2.5) / tanh(0.4)       // linear VCA, unity at g = 1
HI:  OUT = g * 2.5 * tanh(6.0 * IN/2.5)                   // squared up, rounded corners
```

**Middle (folder, odd harmonics)**

| | Name | Details |
|---|---|---|
| K | fold | 0 to 1 |
| I | IN 1, IN 2 | black; summed |
| I | VC | blue |
| O | OUT | black |

Six diode-limited stages summed with alternating sign: a transfer curve that folds back on itself six times each side.

```
c = clamp(knob + VC/5, 0, 1)
x = (IN1 + IN2) * (0.3 + 3.9*c)                          // drive
T = 0.8                                                   // fold spacing, volts
sd(x, θ) = sign(x) * softplus(|x| − θ),  softplus(z) = w*ln(1 + exp(z/w)),  w = 0.03    // diode knee
F = x − 2 * Σ_{k=1..6} (−1)^(k+1) * sd(x, (2k−1)*T)
OUT = railClip(F * 2.5/T)
```
With a ±2.5 V sine: c = 0 gives a clean sine (peak below T); raising c adds the 3rd, 5th ... 13th harmonics in turn.

**Bottom (rectifier chain, even harmonics)**

| | Name | Details |
|---|---|---|
| K | amount | 0 to 1 |
| I | IN 1, IN 2 | black; summed |
| I | VC | blue |
| O | OUT | black |
| O | SQUARED | blue |

Three full-wave rectifier stages with level compensation, brought in one after another.

```
c = clamp(knob + VC/5, 0, 1)
s = clamp((IN1 + IN2)/2.5, −4, 4)
for k = 1..3:
    m = clamp(3*c − (k−1), 0, 1)
    s = (1 − m)*s + m*(2*|s| − 1)                         // each full stage doubles a triangle or sine
OUT = 2.5*s
SQUARED = 2.5 + 2.5*tanh(20*s)                            // squared-up version, 0 to +5 V
```
A triangle in gives a triangle at 2×, 4×, 8× the frequency at c = 1/3, 2/3, 1.

### 5.5 VCFQ, Variable Q Filter

Two-integrator state-variable filter with four simultaneous outputs, voltage-controlled Q, an AGC input, a level-controlled input and a trigger input that pings it. The classic circuit keeps a little resonance at minimum Q and does not self-oscillate by itself (patch BAND to IN for that).

| | Name | Details |
|---|---|---|
| K | FREQ | 20 Hz to 20 kHz exponential. Default 1 kHz |
| K | VC F | attenuverter for F CV, 1 oct/V at full |
| K | Q | default 0.2 |
| K | VC Q | attenuator for Q CV |
| K | GAIN | level of IN, 0 to 2× (unity at centre) |
| I | IN | black, through GAIN: can overload |
| I | AGC IN | black: level falls as Q rises |
| I | TRIG | red |
| I | 1V/OCT | blue |
| I | F CV | blue |
| I | Q CV | blue |
| O | LOW, BAND, HIGH, NOTCH | black |

```
f  = FREQ * 2^(V_1voct + VC_F*V_fcv)
q  = clamp(Qknob + VC_Q*V_qcv/5, 0, 1);   Q = 1.0 * 200^q          // 1 to 200
k  = 1/Q
ping: on TRIG rising edge start p = 5.0; each sample p *= exp(−dt/0.0003)      // 0.3 ms decaying pulse
in = GAIN*IN + AGC_IN * min(1, 2*k) + p
nonlinear SVF (§3.2) at oversampled rate with input `in`, damping k
NOTCH = LOW + HIGH
all outputs through railClip
```
Tests: with a trigger at Q = 100 and 440 Hz, the BAND output rings as a decaying sine whose amplitude falls to 1/e in about Q/(π·f) seconds; unity passband gain at LOW; BAND patched back into IN with GAIN up sustains oscillation; LOW and BAND are 90° apart.

### 5.6 VCFS, Variable Slope Filter

A state-variable filter whose **damping** is the voltage-controlled quantity. At the centre it is a flat 12 dB/octave filter; toward maximum it sharpens and becomes resonant (without ringing like the VCFQ); toward minimum the two poles split apart so the response near cutoff becomes a 6 dB/octave slope.

| | Name | Details |
|---|---|---|
| K | FREQ | 20 Hz to 20 kHz. Default 1 kHz |
| K | VC F | attenuverter |
| K | SLOPE | default centre |
| K | VC SLOPE | attenuverter |
| K | MIX | cross-fade IN 1 ↔ IN 2, linear, default centre |
| I | IN 1, IN 2 | black |
| I | 1V/OCT, F CV, SLOPE CV | blue |
| O | LOW, BAND, HIGH | black |

```
s = clamp(SLOPE + VC_SLOPE*V_slopecv/5, 0, 1)
Q = (s <= 0.5) ? 0.05 * (0.707/0.05)^(2*s)  :  0.707 * (8/0.707)^(2*s − 1)        // 0.05 … 0.707 … 8
in = (1 − MIX)*IN1 + MIX*IN2
nonlinear SVF with k = 1/Q.  BAND output is scaled by k (constant skirt gain) so it does not blow up at low Q.
```
Tests: at s = 0.5 the LOW response is within 0.5 dB of a 2nd-order Butterworth; at s = 0 the LOW response falls at 6 dB/octave (±1 dB) over the two octaves above the −3 dB point; at s = 1 there is a peak of about +18 dB at cutoff.

### 5.7 RESEQ, Resonant Equalizer

Ten fixed band filters. Centres, apart from the outer two, are a major seventh apart so that no musical interval is favoured: **29, 61, 115, 218, 411, 777, 1500, 2800, 5200, 11000 Hz**.

| | Name | Details |
|---|---|---|
| K ×10 | one per band | bipolar, centre = flat |
| K | INPUT | input attenuator, 0 to 1 (audio taper: `x²`), default 0.5 |
| I | IN | black |
| O | OUT | black: all bands, flat when every knob is centred |
| O | COMB ↑ | black: bands 61, 218, 777, 2.8k, 11k only |
| O | COMB ↓ | black: bands 29, 115, 411, 1.5k, 5.2k only |

Knob law, with `p` from −1 (fully anticlockwise) to +1 (fully clockwise); 9 o'clock is p = −0.5 and 3 o'clock is p = +0.5:

```
|p| <= 0.5 :  gainDb = 24*p               (±12 dB),          Q = 1.5
p  >  0.5 :   gainDb = 12 + 24*(p − 0.5)  (to +24 dB),       Q = 1.5 * (40/1.5)^((p − 0.5)*2)     // resonant
p  < −0.5 :   gainDb = −12 − 36*(−p − 0.5) (to −30 dB),      Q = 1.5 * (4/1.5)^((−p − 0.5)*2)     // deeper, narrower rejection

x = INPUT_gain * 4 * IN                    // the stage has gain; the attenuator matters
b_k = BP_k(x)                              // TPT SVF band-pass, unity peak gain, centre f_k, Q above
A_k = 10^(gainDb/20)
OUT    = railClip( (x + Σ_k (A_k − 1) * b_k) / 4 )
COMB ↑ = railClip( Σ_{k = 2,4,6,8,10} A_k * b_k / 4 )
COMB ↓ = railClip( Σ_{k = 1,3,5,7,9}  A_k * b_k / 4 )
```
The bands overlap (Q 1.5 at 0.9 octave spacing), which is intended. A band fully clockwise rings hard on transients and clips at the rails with hot input; that is the instrument. Tests: all knobs centred, OUT equals `INPUT_gain × IN` within 0.1 dB from 20 Hz to 20 kHz; a single band at p = +0.5 gives +12 dB ±0.5 at its centre; comb outputs contain only their five bands.

---

## 6. Voice modules

### 6.1 TWS, Triple Waveshaper

Three identical, independent shapers that turn a **sawtooth** into a sine: the transfer curve is one full sine cycle across the ±2.5 V input range, then flat.

Per section: **I** IN (black), **I** VC (blue: an offset added to the input, which moves the saw along the curve and changes the timbre), **K** VC attenuverter, **O** OUT (black).

```
x = IN + VCknob * VC
u = clamp(x/2.5, −1.5, 1.5)
OUT = 2.5 * ( sin(π*u) + 0.03*sin(3π*u) )           // u beyond ±1.5 holds the end value
```
Oversampled. Test: a ±2.5 V saw gives a sine at the same frequency with THD under 5%; a 0 to 5 V saw gives the same sine when VC supplies −2.5 V. Note in the manual that the NTO's SAW is unipolar and wants an offset from PRC, or use VAR at WAVE = 1.

### 6.2 RING, Ring Modulator

| | Name | Details |
|---|---|---|
| K | MOD | 0 = signal unchanged, centre = amplitude modulation, 1 = ring modulation |
| K | VC MOD | attenuverter |
| K | CARRIER | carrier level; above centre the carrier is rounded off and gains harmonics |
| K | VC CARRIER | attenuverter |
| I | SIGNAL, CARRIER | black |
| I | MOD CV, CARRIER CV | blue |
| O | OUT | black |

```
m  = clamp(MOD + VC_MOD*V_modcv/5, 0, 1)
cl = clamp(CARRIER + VC_CARRIER*V_carcv/5, 0, 1)
c  = tanh( 2*cl * CARRIER_IN/2.5 * 1.2 ) / tanh(1.2)      // unity and nearly clean at cl = 0.5, rounded above
OUT = SIGNAL * ( (1 − m) + m*c )
```
Oversampled. Carrier rejection is ideal (the hardware claims 80 dB).

---

## 7. Control modules

### 7.1 NOI, Noise Source
Outputs only. **O** WHITE (black): Gaussian, σ = 0.8 V, clipped at ±2.5 V. **O** PINK (black): the white source through a −3 dB/octave filter (Paul Kellet's 7-term filter, coefficients re-derived for the actual sample rate or run at a fixed 48 kHz internal rate with interpolation), σ = 0.8 V. **O** S/H SOURCE (blue): white noise through a one-pole 25 Hz low-pass, scaled to σ = 1.6 V, offset +2.5 V, clipped to 0..5 V; sampled by an S&H it gives voltages spread fairly evenly over 0 to 5 V.

### 7.2 ASR, Analog Shift Register
**I** IN (blue), **I** PULSE (red), **O** OUT 1, OUT 2, OUT 3 (blue), **O** PULSE OUT (red, a copy of the squared pulse input, for chaining). On each PULSE rising edge, in this order: OUT 3 = OUT 2, OUT 2 = OUT 1, OUT 1 = IN. Holds without droop. Values clamp to ±RAIL.

### 7.3 COM, Dual Comparator
Per section: **I** + (black), **I** − (black), **K** THRESHOLD (−5 to +5 V, added to the − side, default 0), **O** OUT (red). `OUT = 5 if (V+ > V− + THRESHOLD) else 0`, with 10 mV hysteresis.

### 7.4 STR, Dual Schmitt Trigger
Section A: **I** IN A, **O** OUT A. Section B: **I** IN B, **O** OUT B. Shared: **O** R/S (red). Each section: output goes to +5 V when IN rises above +0.6 V and back to 0 V when it falls below −0.6 V (made for squaring audio). R/S is a flip-flop: set by a rising edge of OUT A, reset by a rising edge of OUT B.

### 7.5 PRC, Dual Processor
Per section: **I** IN 1, IN 2, IN 3 (blue), **K** ×3 attenuverters, **K** OFFSET (−5 to +5 V, default 0), **O** OUT (blue). `OUT = railClip(Σ k_i·IN_i + OFFSET)`. DC coupled; works on audio too.

### 7.6 XFAD, Cross-Fader
**I** IN 1, IN 2 (black), **K** FADE (default centre), **I** FADE CV (blue), **K** GAIN (0 to 1, default 1), **I** GAIN CV (blue), **O** OUT (black).
```
x = clamp(FADE + V_fade/5, 0, 1)
g = clamp(GAIN_CV patched ? GAIN * V_gain/5 : GAIN, 0, 1)
OUT = g * ( cos(π/2 * x)*IN1 + sin(π/2 * x)*IN2 )           // equal power
```

---

## 8. Modules added for panel coverage

### 8.1 VCA2, Dual VCA
Exponential VCAs. Per section: **I** IN (black), **I** VC (blue), **K** GAIN (adds 0 to 5.5 V to the control voltage, default 0), **O** OUT (black).
```
v = VC + 5.5*GAIN
gainDb = 12 * (v − 5)                    // 12 dB per volt, unity at 5 V
if (v < 0.5) gainDb −= 200*(0.5 − v)     // closes completely just below 0.5 V
gain = min(10^(gainDb/20), 2.0)          // cannot exceed ×2
OUT = railClip(gain * IN)
```

### 8.2 MIX, Dual Audio Mixer
Per section: **I** IN 1, IN 2, IN 3 (black) each with **K** level (0 to 1, audio taper `x²`, 0.707 at centre is acceptable), **I** IN 4 (black, unity gain, no knob), **O** OUT (black). `OUT = railClip(Σ)`. DC coupled. No normalling between the sections; patch OUT A into IN 4 of B for six inputs.

### 8.3 DCSM, Dual Channel Stereo Mixer
Per channel: **I** IN (black), **K** GAIN (0 to 1), **I** GAIN CV (blue), **K** PAN (default centre), **I** PAN CV (blue, 0 to 5 V sweeps the full width). Shared: **I** AUX L, AUX R (black, unity, straight to the outputs), **O** LEFT, RIGHT (black).
```
g   = GAIN_CV patched ? GAIN * clamp(V/5, 0, 1) : GAIN           // linear VCA
pan1 = clamp(PAN1 + V_pan1/5, 0, 1)
pan2 = clamp(PAN2 − V_pan2/5, 0, 1)                              // channel 2 pans the opposite way for the same CV
L = Σ g_i * cos(π/2 * pan_i) * IN_i + AUX_L;    R = Σ g_i * sin(π/2 * pan_i) * IN_i + AUX_R
```
Outputs through `railClip`.

### 8.4 PT, Peak & Trough
**I** A, B (blue), **O** PEAK = max(A, B), **O** TROUGH = min(A, B) (blue). An unpatched input is ignored (the output follows the other one). Precision rectifier behaviour: no diode drop.

---

## 9. Tests (`tests/`)

For every module: the behaviour tests listed in its section, plus:
- **Stability:** 10 s of white noise at ±10 V into every input with random knob settings, at 44.1, 48, 96 and 192 kHz: all outputs finite and within ±RAIL (pulse outputs exactly 0 or 5).
- **Silence:** no input, default knobs: non-generator modules output 0 V exactly (no DC, no denormals; enable flush-to-zero or add a guard).
- **Sample-rate independence:** DSG cycle frequency, SSG cycle frequency, NTO pitch, filter cutoffs within 0.5% across the four rates.
- **Self-patch:** DSG END→TRIG, SSG CYCLE→IN on both sides, VCFQ BAND→IN simulated with one sample of delay, run 5 s, check the expected frequency and no blow-up.

In Rack itself (manual, before hand-over): load every module, patch the "Try this" patches of §10, change sample rate while running, save and reload a patch, check CPU meters.

---

## 10. Manual (`docs/MANUAL.md`)

One section per module: what it is, a table of controls, and three or four classic patches written as cable lists. Include at least: DSG as envelope, LFO, clock divider and oscillator; SSG random voltages with NOI; SSG staircase; the "Serge voice" NTO SINE → VCM middle → VCFQ with a DSG envelope into VCM VC and VCA2; VCFQ pinged by a DSG END pulse; RESEQ in a feedback loop through XFAD; ASR canon across three NTOs; the level note from §3.1.

---

## 11. Order of work

1. Repo skeleton, Makefile, CI workflow, shared components (jacks, knob, switches), panel script, one empty module that loads.
2. `Common`, `Slew`, `Svf`, `Oversample`, `Noise` with tests.
3. DSG, SSG (the slew family) with tests.
4. NTO, VCM, TWS, RING.
5. VCFQ, VCFS, RESEQ.
6. NOI, ASR, COM, STR, PRC, XFAD, VCA2, MIX, DCSM, PT.
7. All panels drawn, rendered and inspected, including the label check of §4.1. Manual. Full test pass. CI green on all platforms.
8. Hand-over note: what was built, the contents of `DECISIONS.md`, anything from §12 you could or could not verify, and install steps.

---

## 12. Accuracy ledger

What rests on documentation and what is this spec's own estimate. Where you can reach a schematic or the original datasheet, check the **estimate** items first and correct them (logging each change).

**From sources (Serge 1983 catalog and kit notes, CGS/Ken Stone build notes, Random\*Source docs, builders' measurements):**
- Jack colour code and levels (§3.1), 2.5 V pulse threshold.
- DSG: unity-gain follower with separately VC slopes; linear slopes; trigger and END; no retrigger before the cycle ends; 0 to +5 V envelope; about 4 to 5 kHz maximum when cycling; 1 V/oct input good to about 3%; bipolar VC switchable to rise, fall or both; END high during rise and at rest; all ten listed uses.
- SSG: both sections share one slew design; the stepped side is a slew gated by a roughly 1 ms one-shot on the rising edge of SAMPLE; HOLD freezes the smooth side; VC inputs attenuate only; CYCLE and HOT outputs swing near the rails; COUPLER is 0 to 5 V and high when stepped > smooth; staircase at full RATE in the test patch.
- NTO: outputs and their signal types; 16 Hz to 16 kHz; two calibrated pitch inputs, one with portamento; linear FM with built-in depth VCA and CV; exponential FM; sync to fundamental, harmonics and sub-harmonics.
- VCM: the three sections, HI/LO, odd harmonics 1 to 13 in the middle, three rectifiers giving 2nd, 4th, 8th in the bottom, two inputs on the lower two, squared-up second output.
- VCFQ: four outputs, VC Q, AGC input plus level-controlled input, trigger ringing, a little resonance at minimum Q, oscillation only by patching BAND to IN, 90° between outputs.
- VCFS: three outputs, two inputs with a mix knob, slope 6 dB at minimum, flat 12 dB at centre, sharper and resonant at maximum, VC slope.
- RESEQ: band frequencies, comb assignments, knob zones (±12 dB between 9 and 3 o'clock, resonant above, deeper rejection below), input attenuator, high gain.
- RING: dry → AM → ring control with CV, carrier level with CV and carrier rounding above centre.
- NOI, ASR, COM, STR, PRC, XFAD, 2VCA (12 dB/V, cut-off below 0.5 V, gain limit ×2), MIX (three knobs plus a unity input), DCSM (opposite pan CV sense, aux inputs): functions as described in the catalog.

**Estimates made for this spec (verify if you can):**
- Every knob range and taper constant: DSG `0.25·2^(17.6k)` V/s, SSG `0.5·2^(12k)` V/s, filter ranges 20 Hz to 20 kHz, VCFQ Q 1 to 200, VCFS Q 0.05 to 8, RESEQ band Q values and the +24 / −30 dB extremes.
- Follower gain `G` and the DSG comparator hysteresis.
- SSG triangle amplitude (±2.5 V Schmitt thresholds) and maximum knob rate.
- NTO VAR as a sine-to-saw cross-fade (the sources only say "variable"); soft-sync window 0.70; sine shaper residue; linear FM index; portamento range.
- VCM transfer-curve constants (tanh drives, fold spacing 0.8 V, diode knee) and the stage-by-stage cross-fade used for the bottom section (the hardware may instead shift a bias through the rectifiers).
- VCFS as "variable damping": it matches the documented behaviour at all three knob positions but the circuit was not seen.
- TWS and RING control lists (no jack list was found): TWS has IN, VC with attenuverter, OUT per section; RING as in §6.2.
- STR thresholds and the R/S arrangement; COM threshold range; PRC as three attenuverting inputs plus offset; ASR pulse output.
- PT as a two-input module (documented only as part of a later slope generator).
- Noise amplitudes and the S/H source filter.

**Sources**
- Serge Modular Music Systems catalog and data sheets (1983): https://www.vasulka.org/archive/Artists8/Terperin,Serge/Catalogue.pdf
- CGS / Ken Stone Serge boards, with Serge's kit notes: https://sdiy.info/wiki/CGS_Serge_dual_universal_slope_generator , https://sdiy.info/wiki/CGS_Serge_smooth_and_stepped_generator , https://sdiy.info/wiki/CGS_Serge_resonant_equalizer_(previous_version) , https://sdiy.info/wiki/CGS_Serge_ring_modulator , and the schematics at https://www.elby-designs.com/webtek/cgs/serge/
- Richard Brewster's SSG build and measurements: https://pugix.com/synth/cgs-serge-ssg/
- Serge module reference: https://serge.sublevel9.net/ (filters, CV generators, audio processors, mixers, Animal panel)
- NYU IDM Serge guide (jack-by-jack lists for Random\*Source panels, signal conventions): https://idmnyu.github.io/audiolab/serge.html
- Random\*Source: https://serge-modular.com/serge_panels , https://serge-modular.com/serge_eurorack
