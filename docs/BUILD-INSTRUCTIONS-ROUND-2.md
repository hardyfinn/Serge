# Build instructions, round two: four more Serge modules

**Audience:** an Opus model adding these modules to the existing `Serge` plugin in this repository.
**Owner:** Haer. Fluent in VCV Rack, does not read C++. Build all four in one go, test them yourself, push to `main`, hand over the finished result. No milestone check-ins.

**What to add:** four modules, which brings the plugin to 23.

| Slug | Panel name (title, capitals) | `plugin.json` name | Guide width |
|---|---|---|---|
| `WAD` | WILSON ANALOG DELAY | Wilson Analog Delay (WAD) | 16 HP |
| `FRS` | FREQUENCY SHIFTER | Frequency Shifter (FRS) | 12 HP |
| `BDR` | BI-DIRECTIONAL ROUTER | Bi-Directional Router (BDR) | 12 HP |
| `PDIV` | PULSE DIVIDER | Pulse Divider (PDIV) | 4 HP |

**Read first, in this order:** `docs/BUILD-INSTRUCTIONS.md` (round one; its §0, §3 and §4 are binding here too), `docs/DECISIONS.md` (where the build already departs from round one), then one existing module end to end as a pattern: `scripts/make_panels.py` (the `Panel(...)` blocks), `src/generated/Layout.hpp`, `src/dsp/Ssg.hpp`, `src/modules/SSG.cpp`, `tests/test_slopes.cpp`.

Haer's standing decisions still apply: **as faithful as possible to the original circuits, no modern extras, paperface look with exact text labels, Serge-native levels.**

---

## 0. Rules for this round

1. Everything in round one §0 applies: mono ports, no added controls, no context-menu options, DSP in `src/dsp/` with no Rack includes, no allocation in `process()`, an `isfinite` guard at the end of every `process()`, every param and port named with the panel wording.
2. **Follow the code that exists, not the round-one text, where they differ.** In particular:
   - DSP headers must compile as **C++11** (Rack's plugin flags). Tests build as C++17.
   - Do **not** add `-Isrc` to the Makefile. A header named like one of Rack's (`dsp/common.hpp`) gets shadowed on macOS and Windows. Include with relative paths as the existing modules do.
   - Tests use `tests/check.hpp`, not doctest.
   - Panels and widget positions come from `scripts/make_panels.py`, which writes `res/*.svg` and `src/generated/Layout.hpp`. Add four `Panel(...)` blocks there. Its layout checker must pass with no rule relaxed.
   - The Rack SDK cannot be downloaded in the cloud container. `make test` runs locally; the Rack build runs on GitHub Actions. Push, then read the workflow result and fix until all four platform jobs are green.
3. Do not change the behaviour, layout or parameter ids of the 19 existing modules. Patches saved with version 2.0.0 must load unchanged.
4. New files: `src/dsp/Wad.hpp`, `src/dsp/Frs.hpp`, `src/dsp/Switching.hpp` (BDR and PDIV), `src/modules/WAD.cpp`, `src/modules/FRS.cpp`, `src/modules/Switching.cpp`, `tests/test_round2.cpp`. Register the four models in `src/plugin.hpp` and `src/plugin.cpp`.
5. `plugin.json`: version `2.1.0`. Names exactly as in the table above, so that searching Rack's browser for the short name finds the module. Tags from Rack's list: WAD `Delay`, `Flanger`, `Chorus`; FRS `Effect`, `Ring modulator`, `Oscillator`; BDR `Switch`, `Utility`; PDIV `Clock modulator`, `Logic`.
6. Jack colours and levels per round one §3.1. Pulse inputs fire at +2.5 V rising and re-arm below +2.0 V (`SchmittEdge`).
7. CPU: each module under 1% of one core at 48 kHz with every output patched.
8. Each ambiguity: pick the reading closest to the hardware, add a numbered item to `docs/DECISIONS.md`, list it in the hand-over.

---

## 1. WAD, Wilson Analog Delay

### 1.1 The circuit

A bucket-brigade delay built on two Reticon SAD1024 chips, each of which holds two 512-stage lines. The four lines run in series. Output A is taken after the second line (1024 stages) and output B after the fourth (2048 stages), so **B is always twice the delay of A**. A voltage-controlled clock sets the delay. Three inputs are mixed into the line: two signal inputs with gain knobs, and a third that returns A, B or an external signal as feedback through a control that scales and inverts. Voltage-controlled low-pass filters condition the signal going into the line and coming out of it; the user sets them, they do not follow the clock by themselves. A flanging output mixes the undelayed input with A. A 1 V/oct output reports the clock control voltage so that an external filter can follow the delay.

Catalog figures: delay from 0.5 ms to more than 0.5 s, and more than 5 s for slow signals such as control voltages. The bandwidth shrinks as the delay grows, and a bright signal at a long delay with the filters open comes out "very modulated". That aliasing is part of the instrument and must be present.

A bucket brigade is a sampled system. A line of N stages holds N/2 samples and takes one sample per clock period, so its delay is `N / (2·f_clk)`:

- A: `512 / f_clk` seconds. B: `1024 / f_clk` seconds.
- 0.5 ms at A needs `f_clk` = 1.024 MHz. 5 s at B needs 205 Hz.

### 1.2 Controls and jacks

| Kind | Name | Colour / range | Notes |
|---|---|---|---|
| K | DELAY | 0 to 1, default 0.5 | clockwise is a **longer** delay |
| K | VC DELAY | attenuverter, default 0 | scales DELAY CV; fully clockwise is exactly 1 octave of clock per volt |
| K | FILTER | 0 to 1, default 1 | cutoff of the input and output filters; fully clockwise is open |
| K | VC FILTER | attenuverter, default 0 | scales FILTER CV, 1 octave per volt at the ends |
| K | IN 1 | 0 to 1, default 1 | gain of IN 1 |
| K | IN 2 | 0 to 1, default 0 | gain of IN 2 |
| K | IN 3 | attenuverter, default 0 | feedback: scales and inverts the source chosen by the switch |
| K | FLANGE | 0 to 1, default 1 | depth: amount of A in the FLANGE output |
| S | A / B / AUX | 3 positions, default A | source for IN 3 |
| I | IN 1 | black | main audio input |
| I | IN 2 | black | second input, DC coupled, for audio or control voltages |
| I | AUX | black | external feedback return |
| I | DELAY CV | blue | |
| I | FILTER CV | blue | |
| O | A | black | delayed output |
| O | B | black | delayed output, twice the delay of A |
| O | FLANGE | black | |
| O | 1V/OCT | blue | clock control voltage, for an external filter |

Section titles on the panel: `DELAY`, `FILTER`, `INPUTS`, `OUTPUTS`. The IN 1 and IN 2 knobs sit directly above their jacks, joined by link lines. The switch sits beside the IN 3 knob with its three positions labelled `A`, `B`, `AUX`. No lights.

### 1.3 Model (`src/dsp/Wad.hpp`)

State: `float buf[1024]` (zeroed), write index `w`, clock phase, held values `heldA`, `heldB`, two `Blep`s, an input filter and two output filters (`Svf`, low-pass output, Q = 0.7071, linear), the previous filtered input, the previous output samples of A and B (for feedback), an `Rng`.

Per host sample:

```
// 1. clock
V     = clamp(12.32*(1 − DELAY) + VC_DELAY * V_delaycv, 0, 12.32)      // volts
f_clk = 200 * 2^V                                                      // 200 Hz .. 1.024 MHz
out_1VOCT = V − 6                                                      // 0 V at a 12.8 kHz clock

// 2. filters share one cutoff
fc = clamp(20 * 2^(10*FILTER + VC_FILTER * V_filtercv), 5, min(20000, 0.45*fs))

// 3. input mix
dry = IN1_knob*V_in1 + IN2_knob*V_in2
fb  = switch==A ? lastA : switch==B ? lastB : V_aux        // lastA, lastB = previous sample's A and B outputs
x   = inputFilter.lowpass(dry + IN3_knob*fb, fc)

// 4. bucket brigade: one write per clock tick, zero, one or many per sample
step = f_clk / fs
phase += step
while (phase >= 1) {
    phase −= 1
    a   = phase / step                    // how long before the end of this sample the tick fell, 0 <= a < 1
    xin = x − a*(x − x_prev)              // input at the tick, linear between the last two samples
    newB = buf[w]                         // oldest sample, written 1024 ticks ago
    newA = buf[(w + 512) & 1023]          // written 512 ticks ago
    buf[w] = bbdSat(xin) + 0.001*rng.gaussian()
    w = (w + 1) & 1023
    blepA.add(newA − heldA, a);  heldA = newA
    blepB.add(newB − heldB, a);  heldB = newB
}
x_prev = x

// 5. outputs
A = outFilterA.lowpass(blepA.process(heldA), fc)
B = outFilterB.lowpass(blepB.process(heldB), fc) * 0.891          // −1 dB
FLANGE = railClip(dry + FLANGE_knob * A)
lastA = A;  lastB = B
```

- `bbdSat(v) = 6 * tanh(v/6)`: the line's soft limit. About 1.4% third harmonic at ±2.5 V.
- The outputs are zero-order holds of the stored samples, as on the hardware after its sample-and-hold stages. The `Blep` keeps the steps of that hold from aliasing at the host rate. It is a quality measure, and it must not be replaced by anything that removes the hold itself: when `f_clk` is below the audio band the staircase and its images are the sound of the module.
- No anti-alias filtering beyond the user's FILTER setting. With FILTER open and a long delay, input content above `f_clk / 2` folds down. That is the catalog's "very modulated output" and its "modulation of the input signal by the clock".
- No clock bleed: the Wilson design's noise-cancelling output stage removes it.
- DELAY and FILTER knobs are smoothed (5 ms). The CV paths are not smoothed, so audio-rate modulation of the delay works.
- Feedback uses the previous sample of A or B, which adds one host sample to the loop. That is the same as a cable.
- `Blep` adds one sample of latency. Accept it.
- Reset (`onReset`, sample-rate change): zero the buffer and all states.

### 1.4 Tests

- **Delay time:** an impulse into IN 1 with `f_clk` = 12.8 kHz (DELAY set so V = 6): the peak of A arrives 40 ms later and B 80 ms later, within one clock period plus two host samples. Repeat at V = 3 and V = 9.
- **1 V/oct:** VC DELAY fully clockwise, +1 V at DELAY CV halves both delays within 0.5%. `1V/OCT` out rises by exactly 1 V.
- **Sample-rate independence:** delay times within 0.5% at 44.1, 48, 96 and 192 kHz.
- **B is twice A** at five clock settings.
- **Hold and aliasing are present:** 3 kHz sine in, `f_clk` = 4 kHz, FILTER open: A contains a 1 kHz component (3 kHz folded around half the clock rate) within 6 dB of the input level.
- **Filter cleans it:** same patch with FILTER at 500 Hz: output below −40 dB relative to the open case.
- **Echo:** switch at A, IN 3 at +0.7, delay 40 ms, a 5 ms burst of 1 kHz sine in: the RMS of successive echoes at A falls by a constant ratio between 0.6 and 0.72, and the echoes are spaced by the delay plus at most three host samples. IN 3 at −0.7: successive echoes are inverted.
- **Karplus-Strong:** switch at A, IN 3 = +0.98, `f_clk` = 112.64 kHz (A delay 4.545 ms), a 1 ms noise burst in: A rings at 220 Hz, within 3% and on the flat side (the filters and the one-sample loop add a little delay).
- **Flange:** white noise in, FLANGE = 1, delay 1 ms at A: FLANGE has notches within 5% of 500 Hz, 1.5 kHz and 2.5 kHz, at least 20 dB deep.
- **Stability:** round one §9 noise test with IN 3 at both extremes and each switch position. Outputs stay finite and within ±RAIL.
- **Silence:** with no input and the noise term disabled by a test hook, outputs are exactly 0. (With the noise term on, A and B carry about 1 mV RMS; that is intended.)

---

## 2. FRS, Frequency Shifter

### 2.1 The circuit

A single-sideband frequency shifter with its own quadrature sine oscillator. It does not transpose: it moves every component of the input by the same number of hertz, so harmonic sounds turn inharmonic. The input passes through a phase-difference network that produces two copies 90° apart across the audio band. Each copy is multiplied by one phase of the oscillator; the sum of the two products is the up-shifted signal and the difference is the down-shifted one. UP and DOWN each have an output VCA. The oscillator's 0° and 90° sine outputs are on the panel. A further output gives a compressed copy of the input. The catalog stresses very high carrier rejection and **no squelch circuit**: nothing gates the output at low levels.

Only the basic FRS is built. The FRSX external-carrier version is not.

### 2.2 Controls and jacks

| Kind | Name | Colour / range | Notes |
|---|---|---|---|
| K | FREQ | 0 to 1, default 0.5 | shift frequency, 0.05 Hz to 5 kHz, exponential |
| K | VC FREQ | attenuverter, default 0 | scales FREQ CV, 1 octave per volt at the ends |
| K | UP | 0 to 1, default 1 | level of the UP output |
| K | DOWN | 0 to 1, default 1 | level of the DOWN output |
| I | IN | black | |
| I | FREQ CV | blue | |
| I | 1V/OCT | blue | calibrated |
| I | VC UP | blue | 0 to 5 V, scaled by UP when patched |
| I | VC DOWN | blue | 0 to 5 V, scaled by DOWN when patched |
| O | UP | black | input shifted up by the shift frequency |
| O | DOWN | black | input shifted down by the shift frequency |
| O | COMP | black | compressed copy of the input |
| O | 0° | black | oscillator sine, ±2.5 V |
| O | 90° | black | oscillator sine a quarter cycle ahead of 0°, ±2.5 V |

Section titles: `OSCILLATOR`, `SHIFTER`. The UP and DOWN knobs sit above their VC jacks, joined by link lines. The degree sign is part of the two labels; check that the lettering face has the glyph. No lights.

### 2.3 Model (`src/dsp/Frs.hpp`)

Runs at the oversampled rate `fs_os` from `Oversample.hpp` (factor 4 up to 50 kHz, 2 up to 100 kHz, else 1), because shifting up moves content past the host Nyquist frequency. Upsample IN once; decimate UP and DOWN, each only when patched. The oscillator outputs and COMP are taken at the host rate.

**Oscillator.**
```
f  = clamp(0.05 * 2^(16.61*FREQ + VC_FREQ*V_freqcv + V_1voct), 0.01, 8000)     // Hz
ph += f / fs_os   (wrap to 0..1)
c  = cos(2π ph)        // 90° output = 2.5 * c
s  = sin(2π ph)        // 0° output  = 2.5 * s
```
Compute `s` and `c` once per oversampled sample (a rotating phasor renormalised every block is fine). No BLEP is needed for sines.

**Phase-difference network.** Two chains of six first-order all-pass sections each. Section with pole frequency `p` (Hz):
```
k = π p / fs_os
g = (k − 1) / (k + 1)
y[n] = g*x[n] + x[n−1] − g*y[n−1]
```
Pole frequencies, designed for 90° ± 0.25° from 20 Hz to 20 kHz (opposite-sideband rejection 53 dB):

| Chain | p1 | p2 | p3 | p4 | p5 | p6 |
|---|---|---|---|---|---|---|
| I | 25.05 | 112.62 | 448.11 | 1773.27 | 7228.10 | 55766.88 |
| Q | 7.17 | 55.27 | 225.18 | 890.98 | 3546.26 | 15953.69 |

Chain I leads chain Q by 90°. Use `double` for the section states and coefficients (the lowest poles sit very close to the unit circle). Recompute `g` on sample-rate change. Analog shifters of the period used networks of this size, and their residual opposite sideband, and the loss of quadrature below about 20 Hz, are part of the sound; do not substitute a longer or FIR Hilbert transformer.

**Shifting.**
```
I = chainI(x);  Q = chainQ(x)             // x = upsampled IN
up   = I*c − Q*s
down = I*c + Q*s
```
Ideal multipliers: no carrier leak is added. Unity gain: a sine of amplitude A in gives amplitude A out.

**Output VCAs** (host rate, after decimation; linear):
```
gU = clamp(UP_knob   * (VC_UP patched   ? V_vcup/5   : 1), 0, 1)
gD = clamp(DOWN_knob * (VC_DOWN patched ? V_vcdown/5 : 1), 0, 1)
UP = railClip(gU * up);   DOWN = railClip(gD * down)
```
Knobs smoothed 5 ms.

**COMP** (host rate): a 2:1 feed-forward compressor that levels the input toward ±2.5 V.
```
env  = peak follower of |V_in|, attack 1 ms, release 100 ms
gain = sqrt(2.5 / max(env, 0.025))            // at most ×10
COMP = railClip(V_in * gain)
```

### 2.4 Tests

- **Shift:** 1 kHz sine at ±2.5 V, shift 100 Hz: UP is 1100 Hz within 0.1 Hz at 2.5 V ± 0.1 dB; the 900 Hz component in UP is at least 50 dB down; any 1 kHz or 100 Hz component is at least 80 dB down. DOWN is the mirror image.
- **Band:** repeat with inputs at 30 Hz, 100 Hz, 3 kHz, 10 kHz and 16 kHz, shift 7 Hz: opposite sideband at least 45 dB down at every host rate (44.1, 48, 96, 192 kHz).
- **Not a transposer:** input of 200 + 400 + 600 Hz, shift 50 Hz: UP has 250, 450, 650 Hz.
- **Through zero:** 300 Hz in, shift 500 Hz: DOWN is 200 Hz.
- **No aliasing:** 15 kHz in, shift 5 kHz at 48 kHz host rate: UP (20 kHz) present, nothing above −70 dB below 15 kHz.
- **Oscillator:** 0° and 90° are sines at ±2.5 V, 90° ± 0.5° apart, frequency within 0.1% of the formula at four host rates; +1 V at 1V/OCT doubles it.
- **VCAs:** UP knob 0 silences UP exactly; VC UP at 2.5 V with the knob full gives half amplitude.
- **COMP:** a 0.25 V peak sine comes out at 0.79 V peak ± 5% (2:1); a 2.5 V peak sine at 2.5 V ± 5%.
- **No squelch:** a 5 mV input gives a 5 mV shifted output (no gating).
- Stability and silence tests of round one §9.

---

## 3. BDR, Bi-Directional Router

### 3.1 The circuit

Three identical electronic switches (the 1973 triple router). Each has one single-ended terminal, **B**, and a pair, **A1** and **A2**. A pulse or control voltage at the select input decides which of the pair is connected to B. The switch is a CMOS transmission gate (a CD4007 wired as FET switches), so signal flows either way: one source to either of two destinations, or either of two sources to one destination. It passes signals up to about ±7 V.

### 3.2 The one unavoidable change

A banana jack on the hardware can be a source or a destination. A Rack port is fixed as one or the other. So each section carries **both directions as separate jacks**, driven by the same select input. This adds nothing to what the circuit can do; it is the only way to offer both uses in Rack. Record it in `docs/DECISIONS.md` and explain it in the manual.

### 3.3 Controls and jacks (per section; sections titled `1`, `2`, `3`)

| Kind | Name | Colour | Notes |
|---|---|---|---|
| I | SELECT | red | low or unpatched selects A1, high selects A2 |
| I | B IN | black | router: the source |
| O | A1 OUT | black | router: B IN when A1 is selected, otherwise 0 V |
| O | A2 OUT | black | router: B IN when A2 is selected, otherwise 0 V |
| I | A1 IN | black | selector: first source |
| I | A2 IN | black | selector: second source |
| O | B OUT | black | selector: A1 IN or A2 IN, whichever is selected |

Within each section, draw a sub-box labelled `1 TO 2` around B IN, A1 OUT, A2 OUT and a sub-box labelled `2 TO 1` around A1 IN, A2 IN, B OUT. No knobs, no manual switch (it was a later kit option), no lights.

### 3.4 Model (`src/dsp/Switching.hpp`)

```
// select: level sensitive, with the system's pulse thresholds
if (V_select > 2.5) sel = A2;  else if (V_select < 2.0) sel = A1;      // otherwise unchanged
pass(v) = clamp(v, −7, 7)                                              // CMOS supply limit

A1_OUT = sel==A1 ? pass(V_bin) : 0
A2_OUT = sel==A2 ? pass(V_bin) : 0
B_OUT  = pass(sel==A1 ? V_a1in : V_a2in)
```

- The unselected destination is disconnected on the hardware. An unconnected Serge input reads 0 V, so the unselected output is 0 V.
- Switching is immediate and works at audio rate (chopping one signal with another is a classic use). To keep the jump at the switching instant from aliasing, give each of the three outputs a `Blep`: when `sel` changes, estimate where in the sample SELECT crossed its threshold by linear interpolation between the previous and current SELECT voltages, and call `add(jump, a)` with the size of the jump that output makes. Feed each output through its `Blep::process`. This adds one sample of latency to all three outputs. If SELECT is unpatched, `sel` is A1 and nothing ever switches.
- No on-resistance, crosstalk or charge injection is modelled.

### 3.5 Tests

- SELECT at 0 V: A1 OUT equals B IN (delayed one sample), A2 OUT is 0, B OUT equals A1 IN. SELECT at 5 V: the reverse.
- Hysteresis: SELECT at 2.2 V keeps whichever side was last selected.
- Clamp: a ±10 V input comes out at ±7 V.
- Audio-rate chop: A1 IN = +2.5 V DC, A2 IN = −2.5 V DC, SELECT a 2.1 kHz square at 48 kHz: B OUT is a 2.1 kHz square whose aliases in the audio band are at least 30 dB below the naive (un-corrected) version.
- The three sections are independent.
- Stability and silence tests of round one §9.

---

## 4. PDIV, Pulse Divider

### 4.1 The circuit

One pulse input and seven outputs. The outputs fire on every 2nd, 3rd, 4th, 5th, 6th, 7th and 8th input pulse. Fed with a clock it gives related rhythms, with the odd divisions making polyrhythms. Fed with an audio-rate pulse it gives the first eight sub-harmonics of the input. It is a later addition to the Serge family (a Ken Stone design, built from CMOS counters) and has no knobs.

Serge's own 1979 divider, the Divide-by-N Comparator, is a different module and is not part of this round.

### 4.2 Jacks

| Kind | Name | Colour | Notes |
|---|---|---|---|
| I | IN | red | any signal; counts each rise through +2.5 V |
| O | ÷2, ÷3, ÷4, ÷5, ÷6, ÷7, ÷8 | red | 0 or +5 V |

One column: IN at the top, then an outlined `OUT` block with the seven outputs in order. Labels use the division sign, which is the label on the hardware and counts with `COMB ↑` as a permitted glyph. Tooltips read "Divide by 2" and so on. No lights.

### 4.3 Model (`src/dsp/Switching.hpp`)

Seven independent counters, one per divisor N = 2..8, all starting at 0 with their outputs low.

```
on each rising edge of IN (SchmittEdge):
    for N in 2..8:
        count[N] = (count[N] + 1) % N
        out[N]   = (count[N] == 0) ? 5 : 0
```

- An output goes high on every Nth input pulse and low again on the next input pulse. So ÷2 is a square wave at half the input frequency, and ÷N is high for one input period out of N. The first output pulse comes on the Nth input pulse after reset.
- Outputs are exactly 0 or +5 V, with no band-limiting, as for COM and STR.
- No reset input. `onReset` and patch load clear the counters. The counters are not saved in the patch.

### 4.4 Tests

- 840 input pulses: ÷N has exactly 840/N rising edges for every N, the first on pulse N, and all seven rise together on pulse 840.
- ÷2 duty is 50%; ÷5 is high for one input period in five.
- A ±2.5 V sine at 800 Hz in: ÷2 is a 400 Hz square, ÷8 is 100 Hz.
- A slow ramp crossing 2.5 V once gives exactly one count. A signal that sits between 2.0 and 2.5 V after rising does not count again.
- Stability test of round one §9: outputs are only ever 0 or 5.

---

## 5. Panels

Follow round one §4 and the existing `Panel` helpers exactly: same ground, ink, lettering, jack and knob components, section outlines, link lines, `SERGE` at the bottom. Every control and jack carries the Name from its table above, in capitals. Guide widths are in the first table; widen a module rather than break a spacing rule, and log the change. Render all four panels with `scripts/preview_panels.py` and look at each one before pushing.

Layout notes:
- **WAD:** knobs in two rows of four (DELAY, VC DELAY, FILTER, VC FILTER; then IN 1, IN 2, IN 3, FLANGE). DELAY CV sits under VC DELAY and FILTER CV under VC FILTER with link lines. IN 1 and IN 2 jacks under their knobs. The A / B / AUX switch and the AUX jack sit under IN 3. Outputs A, B, FLANGE, 1V/OCT in an outlined block at the bottom right.
- **FRS:** FREQ and VC FREQ above FREQ CV and 1V/OCT, with 0° and 90° in the oscillator block. UP and DOWN knobs above VC UP and VC DOWN. IN at the lower left; UP, DOWN, COMP in an outlined output block.
- **BDR:** three sections stacked top to bottom, each one row: SELECT, then the `1 TO 2` box, then the `2 TO 1` box.
- **PDIV:** see §4.2.

---

## 6. Manual

Add four sections to `docs/MANUAL.md` in the style already there (what it is, a control table, a "Try this" list), add four rows to "The modules" table, and add at least these recipes to "More patch recipes":

- **Echo:** WAD switch at B, IN 3 around 2 o'clock, FILTER at 1 o'clock. Note that B is the long tap.
- **Plucked string:** a short noise burst (NOI WHITE through a VCA2 opened by a fast DSG) into WAD IN 1, switch at A, IN 3 nearly full, DELAY short, pitch into DELAY CV with VC DELAY fully clockwise. FILTER sets the damping.
- **Flanger and chorus:** a slow DSG or SSG triangle into DELAY CV, listen at FLANGE; for chorus mix A with the dry sound in MIX.
- **Tracking the filter:** WAD 1V/OCT → WAD FILTER CV with VC FILTER fully clockwise, so the filters follow the clock and long delays stay clean; or into a VCFQ 1V/OCT.
- **Delaying a control voltage:** an envelope into IN 2, read it at B.
- **Bell from a shifter:** NTO SINE or TRI → FRS IN, shift by tens of hertz, mix UP and DOWN.
- **Barber-pole feedback:** FRS UP → WAD IN 1, WAD A → MIX with the dry sound → FRS IN, shift of 1 to 5 Hz. Each echo is a little higher than the last.
- **Stereo drift:** shift below 1 Hz, UP to DCSM 1 and DOWN to DCSM 2, panned apart.
- **Alternating notes:** two sequencer rows or two ASR outputs into BDR A1 IN and A2 IN, a clock divided by PDIV ÷2 into SELECT, B OUT to NTO 1V/OCT.
- **Chopping:** two different sounds into A1 IN and A2 IN, an NTO SAW into SELECT.
- **Patch that rewires itself:** B IN from an envelope, A1 OUT to a filter and A2 OUT to a wavefolder, SELECT from SSG COUPLER.
- **Polyrhythm:** one clock into PDIV, ÷3 and ÷4 pinging two VCFQs.
- **Sub-harmonics:** NTO SAW into PDIV IN, mix ÷2, ÷3 and ÷4 in MIX (through PRC with OFFSET at −2.5 V to centre them).

Explain in the BDR section why each section has two sets of jacks (§3.2), and in the WAD section that DELAY clockwise is longer while a positive DELAY CV with VC DELAY clockwise is shorter, as for pitch.

Then regenerate `docs/Serge-Manual.pdf` with `scripts/make_manual_pdf.sh` and commit it. Copy the new PDF to `/mnt/project-files/serge-vcv/Serge-Manual.pdf` as well.

---

## 7. Order of work

1. Read the files listed at the top. Run `make test` and `make panels` to confirm the tree is clean.
2. `Switching.hpp` (BDR, PDIV) with tests.
3. `Frs.hpp` with tests.
4. `Wad.hpp` with tests.
5. Panels for all four; layout checker passes; previews inspected.
6. Module wrappers, registration, `plugin.json`.
7. Manual, PDF, `README.md` module count, `docs/DECISIONS.md`, `docs/CREDITS.md`.
8. Push to the working branch, get all CI jobs green, then fast-forward `main`.
9. Hand-over note: what was built, new `DECISIONS.md` items, which §8 estimates you could or could not verify, and the link to the Actions run with the `.vcvplugin` files.

---

## 8. Accuracy ledger

**From sources**
- WAD (Serge catalog text; CGS208 build notes and test procedure; a builder's log): two SAD1024 chips; A after the second line and B after the fourth, B twice A; 0.5 ms to over 0.5 s, over 5 s for slow signals; bandwidth narrows as delay grows and a bright signal at a long delay is "very modulated"; IN 1 and IN 2 with gain controls, IN 2 suited to control voltages; IN 3 fed through a switch from A, B or the AUX jack, with a control that scales and inverts; voltage-controlled filters on the input and the outputs, set by the user; a flanging output with a depth control; a 1 V/oct output meant for external filters; a noise-cancelling output stage; one builder found B lower in level than A.
- FRS (Serge catalog text): built-in oscillator with 0° and 90° sine outputs; output VCAs for UP and DOWN; a compressed output of the input; no squelch; high carrier rejection; shifts by a fixed number of hertz; an external-carrier version existed separately.
- BDR (1973 board notes, CGS88 notes): three sections; one terminal routed to either of two, or either of two to one, by a pulse or control voltage level; true bi-directional CMOS switches (CD4007); passes about ±7 V; terminals named A1, A2 and B on the 1973 board; a manual switch was only a kit option.
- PDIV (NYU IDM Serge guide, Serge module reference): one pulse input; outputs on every 2nd to 8th pulse; sub-harmonics at audio rate; a Ken Stone design.

**Estimates made for this spec (verify if you can)**
- WAD: every panel name other than IN 1, IN 2, IN 3, AUX, A, B and the idea of a flange depth control; the existence of separate DELAY CV and FILTER CV jacks with attenuverters; the direction of the DELAY knob and of its CV; the clock range (200 Hz to 1.024 MHz, derived from the catalog delay figures and the stage count); the 0 V point of the 1V/OCT output; filter type, order and range; IN 3 range of ±1; the −1 dB level of B; the saturation curve and the 1 mV noise floor; FLANGE as dry plus A at unity.
- FRS: **the whole control list.** No jack-by-jack list of the original panel was found, only the catalog's description of its features. The frequency range, exponential control with a 1V/OCT input, the knob-times-CV VCA law, the 6 + 6 pole phase network and its band, and the COMP compressor law are all this spec's reconstruction.
- BDR: the SELECT threshold (the system's 2.5 V; the CGS notes only say it is higher than usual CGS gates); A1 as the resting side; 0 V at the unselected output; the two-direction jack arrangement of §3.2.
- PDIV: output pulse width (one input period) and the count at which the first pulse appears; no reset.

**Sources**
- Serge catalog descriptions: https://serge.sublevel9.net/p/wilson-analog-delay.html , https://serge.sublevel9.net/p/audio-processors.html , https://serge.sublevel9.net/p/control-voltage-generators-modifiers.html , and the 1983 catalog at https://www.vasulka.org/archive/Artists8/Terperin,Serge/Catalogue.pdf
- WAD builds: https://sdiy.info/wiki/CGS_Serge_Wilson_analog_delay , https://elby-designs.com/webtek/cgs/serge/cgs208/cgs208-wad.html , https://modularsynthesis.com/serge/wad/wad.htm
- BDR: https://sdiy.info/wiki/CGS_Serge_triple_bi-directional_router , https://sdiy.info/wiki/Ken_Stone/1973_classic_Serge/Serge_bi-directional_router , https://elby-designs.com/webtek/cgs/serge/cgs88/cgs88_bds.html
- PDIV and the Divide-by-N Comparator: https://idmnyu.github.io/audiolab/serge.html
