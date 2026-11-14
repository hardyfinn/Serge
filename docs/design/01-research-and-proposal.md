# Serge for VCV Rack: research and proposed module set

Draft 1, 2026-11-14. Nothing here is decided until Haer picks.

## What makes a Serge a Serge

Serge Tcherepnin's system (CalArts, from 1973) differs from Moog-style synths in a few ways that should drive the design:

- **Patch-programmable building blocks.** Modules are low-level functions, not finished instruments. One slope generator becomes an envelope, LFO, VCO, slew, envelope follower, subharmonic divider or pulse delay depending on how it is patched. Self-patching (output back into own input) is the normal way to use it.
- **No hard line between audio and CV.** Everything runs from sub-audio to audio rate, DC coupled.
- **Timbre from waveshaping and resonance** more than from subtractive filtering: Wave Multipliers, Triple Waveshaper, Resonant EQ.
- **Banana jacks, colour coded.** Blue = unipolar CV, black = bipolar/audio, red = gate/trigger, with stackable outputs. In VCV this becomes coloured port rings, and outputs already stack.
- **Paperface look.** Geometric symbols on the panel instead of text labels.

## What already exists in VCV Rack

Checked the VCV Library search for "serge" and a community thread on the subject.

| Serge function | Existing VCV module | Verdict |
|---|---|---|
| DUSG | Befaco Rampage (DUSG descendant), NYSTHI DAHD | Close cousin, not the Serge behaviour or layout |
| Programmer | NYSTHI Programmer + Nudger | Covered reasonably |
| Wave folding | TriggerFish Wavefold Oscillator (has a "Serge" folder mode) | One section only, inside an oscillator |
| Smooth & Stepped Generator | none found | Gap (community thread says the same) |
| Wave Multipliers (all three sections) | none found | Gap |
| Resonant EQ, Variable Slope VCF, NTO, TKB, Wilson Delay, ASR as Serge drew them | none found | Gap |

No module in the library carries Serge in its title. So a coherent Serge set is genuinely new ground, not a duplicate.

Sources: [VCV Library search](https://library.vcvrack.com/?query=serge), [community thread](https://community.vcvrack.com/t/using-rack-modules-to-emulate-the-functionality-of-hardware-serge-modules/4614), [Wikipedia: Serge synthesizer](https://en.wikipedia.org/wiki/Serge_synthesizer), [Random*Source Serge](https://randomsource.net/serge_euro), [SFU Serge Modular manual (Brazeau)](https://summit.sfu.ca/_flysystem/fedora/2023-01/etd22315--jean-brazeau-SFU_Serge_Modular_Manual.pdf), [Clockface Serge guide](https://en.clockfacemodular.com/blogs/waveguide/serge-intro).

## Proposed set: 12 modules

### Core (the ones that make it a Serge)
1. **DUSG, Dual Universal Slope Generator.** Two slopes, each with rise, fall, VC for rise/fall/both, exp/lin, trigger in, gate out, signal in. Must work at audio rate and track 1V/oct well enough to be a VCO. Cycles when self-patched.
2. **SSG, Smooth & Stepped Generator.** Smooth side: VC slew that turns into track-and-hold, cycles when patched. Stepped side: sample-and-hold with slew. Coupler output comparing the two. The classic Serge random and chaos source.
3. **NTO, New Timbral Oscillator.** Sine, triangle, saw, variable waveform with CV, linear and exponential FM, portamento, sync. Low range for LFO use.
4. **Wave Multipliers.** Three independent sections: (top) VC squarer and soft clipper, (middle) multi-stage folder giving odd harmonics, (bottom) full-wave rectifier chain giving even harmonics, with two inputs.
5. **VCFQ, Variable Q Filter.** State variable: low, band, high, notch at once, VC resonance, trigger input for pinging, AGC input.
6. **Resonant Equalizer.** Ten fixed bands spaced at major sevenths so that no musical interval is favoured, plus the two comb outputs. Formant and feedback machine.

### Voice completion
7. **Triple Waveshaper + Ring.** Three saw-to-sine shapers (also good on anything else), and the Serge ring modulator with its VC blend between plain and modulated signal.
8. **Wilson Analog Delay.** Bucket-brigade style delay with VC time and feedback: flanging, chorus, pitch wobble, Karplus-Strong plucks.
9. **VCFS, Variable Slope Filter.** The filter slope itself is voltage controlled, from flat to steep. Unusual and has no VCV equivalent.

### Control and logic
10. **TKB, Touch Keyboard Sequencer.** 16 stages by 4 rows, touch pads (clickable, and CV/gate selectable), A/B/C/D outputs, key voltage, random and vertical stage selection, up/down, reset per stage.
11. **Random and Shift.** Noise source (white, pink, S/H source), random smooth and stepped voltages, and the Analog Shift Register, in one panel.
12. **Utility panel.** Dual comparator with Schmitt trigger, dual processor (attenuvert, offset, mix), and a cross-fader/VCA. The glue that Serge patches always need.

### Candidates if the count goes up
Frequency Shifter, Phaser, Extended ADSR, Dual Transient Generator, Pulse Divider and Boolean logic, Quantizer, Precision VCO as a separate module, Quad VCA/Gator.

## Design stance I would take by default

- **Faithful behaviour, modern conveniences.** Model what the circuits do (including how they misbehave when self-patched and at audio rate), keep Serge's panel logic, but add things hardware could not: polyphony where cheap, oversampling on the shapers, and a context menu for range options.
- **Paperface-inspired panels** with blue/black/red port colouring.
- **Naming.** "Serge" is a trademark in use (Serge Modular, Random*Source). Fine for a personal plugin. If this ever goes to the public VCV Library the plugin and modules should carry their own names with "inspired by" in the description.

## Open questions for Haer

1. Which modules, and how many?
2. Faithful recreation, or Serge as a starting point with new ideas mixed in?
3. Should this thread build the plugin, or write build instructions as before?
4. Which repo? None exists for this yet.
