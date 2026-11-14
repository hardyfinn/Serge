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
