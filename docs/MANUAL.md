# Serge for VCV Rack: manual

## Read this first: levels and jacks

These modules use the levels of a real Serge system, not Rack's usual ones.

| Jack colour | Signal | Level |
|---|---|---|
| **Black** | audio, bipolar | about ±2.5 V (5 V peak to peak) |
| **Blue** | control voltage, unipolar | 0 to +5 V |
| **Red** | pulse, gate, trigger, logic | 0 or +5 V |
| **Yellow** | SSG CYCLE and HOT COUPLER | swings to about ±10.5 V |

- Pitch inputs are 1 V per octave.
- A red input fires when its voltage rises through +2.5 V. Any signal can drive it, which is how Serge patches turn audio and slopes into clocks.
- Any output can go into any input. Patching a module's output back into its own input is normal Serge practice and is safe here.
- Rack's own modules use 10 V peak-to-peak audio and 10 V gates. Next to them Serge outputs look half-size, and Rack oscillators drive Serge shapers hard. That is expected: use PRC, MIX or VCA2 to scale between the two worlds.
- Every port and knob has a tooltip with its full name.

## The modules

| Short name | Module | What it is |
|---|---|---|
| DSG | Dual Universal Slope Generator | envelopes, LFOs, oscillator, slew, clock divider |
| SSG | Smooth & Stepped Generator | slew, track and hold, sample and hold, random |
| NTO | New Timbral Oscillator | the main audio oscillator |
| VCM | Wave Multipliers | squarer, wavefolder, rectifier chain |
| VCFQ | Variable Q VCF | multimode filter that can ring |
| VCFS | Variable Slope VCF | filter with a variable slope |
| RESEQ | Resonant Equalizer | ten fixed resonant bands |
| TWS | Triple Waveshaper | turns sawtooth into sine |
| RING | Ring Modulator | ring and amplitude modulation |
| NOI | Noise Source | white, pink and slow random |
| ASR | Analog Shift Register | three-stage sample and hold |
| COM | Dual Comparator | turns levels into pulses |
| STR | Dual Schmitt Trigger | squaring and a flip-flop |
| PRC | Dual Processor | mixing, inverting, offsets |
| XFAD | Cross-Fader | fade between two signals |
| VCA2 | Dual VCA | two exponential VCAs |
| MIX | Dual Audio Mixer | two small mixers |
| DCSM | Dual Channel Stereo Mixer | stereo mixer with panning |
| PT | Peak & Trough | higher and lower of two signals |

---

## DSG: Dual Universal Slope Generator

Two identical sections. Each one is a voltage follower whose upward speed (RISE) and downward speed (FALL) you set separately. A trigger makes it rise to +5 V and fall back: an envelope. Its END output goes low while it falls, so feeding END back to TRIG makes it cycle.

| Control | |
|---|---|
| RISE, FALL | time for a 5 V slope, from 20 s (anticlockwise) to 100 µs |
| VC | how much VC IN changes the slopes, ± 1 octave per volt at the ends |
| RISE / BOTH / FALL | which slope VC IN changes |
| IN | the voltage the output follows |
| TRIG | starts one rise-and-fall. Ignored while a cycle is running |
| 1V/OCT | speeds both slopes, calibrated |
| OUT | the slope, 0 to +5 V for an envelope |
| END | +5 V while rising and at rest, 0 V during the fall |

Try this:
- **Envelope:** a clock or gate into TRIG, OUT to a VCA2 VC. A gate into IN instead gives attack, sustain at the gate level, and release.
- **LFO and oscillator:** END → TRIG. Turn RISE and FALL down for a slow triangle, both fully up for about 4.8 kHz. Turn one down for a sawtooth. Pitch it from 1V/OCT.
- **Clock divider:** a clock into TRIG, then slow RISE and FALL until each cycle spans two or three clock periods. END now pulses on every second or third clock.
- **Pulse delay:** a trigger into TRIG. END rises again one rise-plus-fall time later.
- **Portamento:** a sequencer into IN, OUT to a pitch input. RISE sets the glide up, FALL the glide down.
- **Envelope follower:** audio into IN, RISE fully up, FALL around noon.
- **Curved slopes:** OUT → VC IN, VC turned up or down.

## SSG: Smooth & Stepped Generator

Two slew sections and a comparator. SMOOTH follows its IN at the speed RATE allows, and freezes while HOLD is high. STEPPED only moves during a 1 ms window after each SAMPLE pulse, so each pulse moves it toward IN by at most RATE × 1 ms. COUPLER is high whenever STEPPED is above SMOOTH.

| Control | |
|---|---|
| RATE | slew speed, 0.5 V/s to 2 kV/s |
| VC | how much VC IN speeds the slew (no inversion) |
| HOLD (smooth) | high freezes the output: track and hold |
| SAMPLE (stepped) | each rising edge opens the 1 ms window |
| CYCLE | flips between ±10.5 V when OUT passes ±2.5 V |
| COUPLER, HOT COUPLER | stepped > smooth: 0/+5 V and ±10.5 V |

Try this:
- **Triangle LFO:** smooth CYCLE → smooth IN. RATE fully up gives about 200 Hz.
- **Staircases:** a clock into SAMPLE, stepped CYCLE → stepped IN. Lower RATE for more, smaller steps.
- **Wandering random voltages:** NOI WHITE → stepped IN, a clock into SAMPLE, RATE low. Each step moves only part of the way.
- **Classic sample and hold:** as above with RATE fully up.
- **Smooth random:** stepped OUT → smooth IN, slow smooth RATE.
- **Rhythms from the coupler:** both sections cycling at different rates, COUPLER into a DSG TRIG.

## NTO: New Timbral Oscillator

| Control | |
|---|---|
| FREQ, FINE | 16 Hz to 16 kHz, FINE ± half an octave |
| 1V/OCT | calibrated pitch input |
| 1V/OCT PORT | calibrated pitch input with its own glide, set by PORT |
| VC FREQ, FREQ CV | extra exponential pitch control |
| FM, FM VC, LIN FM | linear FM: LIN FM is the modulator, FM and FM VC set the depth |
| WAVE, VC WAVE, WAVE CV | shape of the VAR output, from sine to saw |
| SYNC | soft sync: a rising edge restarts the cycle if it is at least 70% through |
| SAW, TRI | 0 to +5 V |
| SINE, VAR | ±2.5 V |

Linear FM keeps the pitch steady as the depth changes, so it gives clean bell and metal timbres. For sync, tune the NTO slightly flat of the master, or flat of a harmonic of it.

## VCM: Wave Multipliers

Three separate shapers.
- **1:** HI / LO switch. LO is a plain VCA. HI squares the signal up with rounded corners. VC is the level.
- **2:** the folder. As FOLD (plus VC) rises, a sine gains the 3rd, 5th and so on up to the 13th harmonic. IN 1 and IN 2 are mixed first.
- **3:** three full-wave rectifiers brought in one after another. A triangle or sine comes out at 2×, 4× then 8× its frequency. SQUARED is a squared-up 0 to +5 V copy.

Try this: NTO SINE → VCM 2 IN 1, a DSG envelope into VCM 2 VC: the brightness follows the envelope.

## VCFQ: Variable Q VCF

Four outputs at once: LOW, BAND, HIGH, NOTCH. Q runs from 1 to 200.
- **IN** goes through GAIN. Turn it up to overload the filter on purpose.
- **AGC IN** turns its level down as Q goes up, so high Q stays clean.
- **TRIG** pings the filter: with high Q it rings like a struck object (wood blocks, bells, clicks).
- For a sine oscillator, patch BAND → IN and raise GAIN. LOW and BAND are 90° apart.

## VCFS: Variable Slope VCF

LOW, BAND and HIGH outputs. SLOPE at the centre is a smooth 12 dB/octave filter. Anticlockwise the slope softens to 6 dB/octave. Clockwise it sharpens and resonates (about +18 dB at the end) but does not ring like the VCFQ. MIX fades between IN 1 and IN 2. Try a DSG envelope into F CV for a low-pass gate, or BAND → IN 2 with MIX for a comb.

## RESEQ: Resonant Equalizer

Ten bands at 29, 61, 115, 218, 411, 777, 1500, 2800, 5200 and 11000 Hz. Most are a major seventh apart, so no musical interval is favoured. With every knob at 12 o'clock, OUT equals IN.
- 9 to 3 o'clock: cut or boost up to 12 dB.
- Past 3 o'clock: the band turns resonant and rings.
- Below 9 o'clock: deeper, narrower cut.
- COMB ↑ carries only bands 61, 218, 777, 2.8k and 11k. COMB ↓ carries the other five.
- INPUT sets the level in. The stage has gain, so hot signals clip.

Try this: RESEQ OUT → XFAD IN 2, the dry sound into XFAD IN 1, XFAD OUT back into RESEQ IN with a couple of bands past 3 o'clock. That is a resonant feedback loop.

## TWS: Triple Waveshaper

Three shapers that turn a ±2.5 V sawtooth into a sine. VC adds an offset, which slides the wave along the curve and changes the timbre. The NTO's SAW is 0 to 5 V, so feed it with VC set to give −2.5 V (PRC OFFSET into VC), or use NTO VAR with WAVE fully up.

## RING: Ring Modulator

MOD runs from the dry SIGNAL, through amplitude modulation at the centre, to full ring modulation. CARRIER sets the carrier level: above the centre the carrier is rounded off and adds harmonics of its own.

## NOI: Noise Source

WHITE and PINK are ±2.5 V noise. S/H SOURCE is slow random voltage from 0 to 5 V, made for sampling with the SSG.

## ASR: Analog Shift Register

Each PULSE: OUT 3 takes OUT 2, OUT 2 takes OUT 1, OUT 1 samples IN. PULSE OUT passes the pulse on to a second ASR. Try three NTOs on OUT 1 to OUT 3 for a canon.

## COM: Dual Comparator

OUT is +5 V when + is above − plus THRESHOLD. Use it to square waves, derive pulse-width modulation (a triangle into +, a CV into −), or fire events when a level is crossed.

## STR: Dual Schmitt Trigger

OUT A and OUT B square their inputs with hysteresis (switch at ±0.6 V). R/S is a flip-flop: set by A, reset by B.

## PRC: Dual Processor

Each half adds three inputs, each through a knob from −1 to +1, plus an OFFSET from −5 to +5 V.

## XFAD: Cross-Fader

Equal-power fade from IN 1 to IN 2 by FADE plus FADE CV. GAIN, scaled by GAIN CV when patched, sets the output level.

## VCA2: Dual VCA

Exponential VCAs: 12 dB per volt, unity at 5 V, shut below 0.5 V, never more than ×2. GAIN adds up to 5.5 V to VC, so with nothing in VC it works as a level knob.

## MIX: Dual Audio Mixer

Three inputs with level knobs and IN 4 at unity. Patch OUT A into IN 4 of B for a six-input mixer.

## DCSM: Dual Channel Stereo Mixer

Two channels, each with GAIN (scaled by GAIN CV, 0 to 5 V) and PAN (offset by PAN CV). The same CV into both PAN CV inputs pans the two channels in opposite directions. AUX L and AUX R go straight to the outputs.

## PT: Peak & Trough

PEAK is the higher of A and B, TROUGH the lower. With one input patched both outputs follow it.

---

## A first Serge voice

1. NTO SINE → VCM 2 IN 1.
2. VCM 2 OUT → VCFQ IN.
3. VCFQ LOW → VCA2 A IN, VCA2 A OUT → your audio interface (through a Rack VCA or mixer if it is too quiet).
4. DSG A: a clock into TRIG, RISE fully up, FALL around 10 o'clock. OUT → VCM 2 VC and VCA2 A VC.
5. SSG stepped: NOI S/H SOURCE → IN, the same clock into SAMPLE, OUT → NTO 1V/OCT.
6. Play with FOLD, VCFQ Q and the DSG FALL.

---

## More patch recipes

Each recipe starts from an empty patch. "Clock" means any steady pulse: a Rack clock module, or a DSG cycling with END → TRIG. Send the final output to your audio interface through a Rack mixer if it is too quiet.

### 1. Struck percussion from a pinged filter

1. A clock → VCFQ TRIG.
2. VCFQ Q near maximum, FREQ to taste. Take BAND out.
3. For woodblocks, set FREQ high and Q a little lower. For bells and gongs, set Q fully up.
4. For random tuned hits, SSG stepped: NOI S/H SOURCE → IN, the same clock → SAMPLE, OUT → VCFQ 1V/OCT.

### 2. Bouncing ball

A burst of hits that speeds up and dies away, like a dropped ball.

1. DSG A: END A → TRIG A, so it cycles. RISE and FALL around 1 o'clock.
2. DSG B: a slow trigger or a manual button → TRIG B. RISE B fully up (fast), FALL B slow, around 9 o'clock.
3. OUT B → VC IN A. Switch A to BOTH and turn VC A anticlockwise, so A slows down while B is high.
4. END A → VCFQ TRIG, VCFQ BAND to the output. Each new trigger into B drops the ball again.

### 3. Two-slope chaos

Two DSGs that bend each other's speed. They never quite repeat.

1. DSG A: END A → TRIG A. DSG B: END B → TRIG B. Set both to slow cycles at different rates.
2. OUT A → VC IN B, OUT B → VC IN A. Turn both VC knobs part way, one clockwise and one anticlockwise.
3. Use OUT A and OUT B as tangled LFOs, and END A and END B as uneven rhythms.
4. Raise both pairs of RISE and FALL into audio range for a rough, chaotic oscillator.

### 4. FM bell

1. NTO 1 is the carrier: SINE → VCA2 A IN.
2. NTO 2 is the modulator: SINE → NTO 1 LIN FM. Tune NTO 2 to an odd ratio of NTO 1, such as 1.4 or 3.5 times, for metal and bell tones.
3. DSG A: a clock → TRIG A, RISE fully up, FALL slow. OUT A → NTO 1 FM VC and VCA2 A VC.
4. The strike is bright, and the tone darkens as it fades. FM sets the brightest point.

### 5. Sync sweep

1. NTO 1 SAW → NTO 2 SYNC. NTO 1 sets the pitch you hear.
2. Tune NTO 2 a little flat of NTO 1, and take its VAR output with WAVE near saw.
3. DSG A envelope OUT → NTO 2 FREQ CV, VC FREQ up. Each trigger sweeps the harmonics.
4. Play both oscillators from the same pitch source through their 1V/OCT inputs.

### 6. Octaves from the rectifiers

1. NTO TRI → VCM 3 IN 1.
2. A slow DSG or SSG LFO → VCM 3 VC. As it rises, the tone climbs through 2×, 4× and 8× the frequency.
3. Mix VCM 3 OUT with the plain NTO TRI in MIX for an organ-like stack.

### 7. Pulse-width modulation

Serge has no pulse oscillator. You make one from a comparator.

1. NTO TRI → COM A +.
2. A slow LFO (DSG A cycling, or SSG smooth CYCLE) → COM A −.
3. COM A OUT is a pulse whose width follows the LFO.
4. The pulse runs from 0 to +5 V. To centre it on 0 V, put it through PRC A IN 1 and set OFFSET to −2.5 V.

### 8. Canon from the shift register

1. NOI S/H SOURCE → ASR IN, a clock → ASR PULSE.
2. ASR OUT 1, OUT 2 and OUT 3 → the 1V/OCT inputs of three voices. Each voice plays the tune one step after the last.
3. With only one NTO, use the voice from "A first Serge voice" on OUT 1, and pitch two pinged VCFQs (recipe 1) from OUT 2 and OUT 3.
4. Patch ASR PULSE OUT into a second ASR for a six-voice round.

### 9. Talking formants

1. NTO SAW → RESEQ IN.
2. Turn two or three neighbouring bands past 3 o'clock so they ring, and pull the others below 9 o'clock.
3. Change which bands are raised to move between vowel-like sounds.
4. For a wide stereo image, RESEQ COMB UP → DCSM 1: IN and COMB DOWN → DCSM 2: IN, PAN 1 left and PAN 2 right.

### 10. Wandering stereo

1. SSG smooth: NOI S/H SOURCE → Smooth IN, Smooth RATE low.
2. Smooth OUT → DCSM 1: PAN CV and 2: PAN CV.
3. Two different sounds into DCSM 1: IN and 2: IN. They drift around each other, always in opposite directions.

### 11. Auto-wah from an envelope follower

1. Any audio, such as a drum loop or a Rack oscillator, → DSG A IN and VCFQ IN.
2. RISE A fully up, FALL A around noon. OUT A now follows the loudness.
3. OUT A → VCFQ F CV, with VC F up and some Q. Take BAND or LOW. Louder playing opens the filter.

### 12. Shapes from Peak & Trough

1. Two LFOs at different rates, such as DSG A and SSG smooth CYCLE → PT A and B.
2. PEAK and TROUGH give new, lopsided shapes that change as the two LFOs drift apart.
3. PEAK → NTO FREQ CV for a pitch that never falls below whichever LFO is higher.
