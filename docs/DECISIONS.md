# Decisions

Where the build departs from, or fills a gap in, `docs/BUILD-INSTRUCTIONS.md`. Each one picks the reading closest to the hardware or to what the spec's own tests require.

## Models

1. **Slew follower gain.** The spec's estimate `G = 50000 /s` made the DSG settle over several samples at the end of each slope, so self-cycling topped out near 3.4 kHz instead of the documented 4 to 5 kHz. `G` is now `400000 /s`. The DSG now cycles at about 4.8 kHz at full knobs and tracks 1V/OCT within 0.6% over four octaves.
2. **Filter saturation.** The spec's `otaSat(v) = 6·tanh(v/6)` applied to the integrator states every sample acts as heavy extra damping: a VCFQ set to Q 100 decayed 30 times too fast. The saturation is now linear up to ±4 V and soft-limits to ±6 V. Normal levels keep their exact Q and gain; overload and self-oscillation stay bounded.
3. **Oversampling filters.** 32 taps per phase (128 at 4×), Blackman-windowed, cutoff at 0.42 of the base rate. The NTO saw at 2.1 kHz now has aliases below −90 dB in the audio band (spec asked for −70 dB).
4. **NTO triangle corners.** No polyBLAMP. With 4× oversampling the triangle's aliasing is already far below the saw's. All four outputs get polyBLEP corrections at the wrap and at sync resets.
5. **NTO soft sync.** As specified, a sync edge resets the phase only when the slave is at least 70% through its cycle. So the slave locks when it is tuned slightly flat of the master (or of a harmonic or sub-harmonic of it), as on the hardware. Tested at 1×, 2× and 0.5×.
6. **Ring modulator** output is clipped at the rails inside the model.
7. **Pink noise** uses Kellet's coefficients at every sample rate (they were designed for 44.1 kHz). The slope stays close to −3 dB/oct across the audio band at higher rates. Level is normalised to 0.8 V RMS.
8. **Dual VCA** is not oversampled: apart from the rail clip it is linear.
9. **Mixer levels** (MIX) use an audio taper, `knob²`.
10. **C++ standard.** Rack's build compiles plugins as C++11, so the models avoid C++17 features. The host tests build as C++17.

## Panels

11. **Widths.** Some modules needed more room than the spec's guide widths to meet its spacing rules with every label in place: DSG 14 HP (guide 12), VCM 14 (12), VCFQ 12 (10), VCFS 12 (10), TWS 10 (8), PRC 14 (10), MIX 14 (10). The rest match.
12. **Lettering face.** No stencil or DIN face was available offline, so labels use DejaVu Sans Bold, condensed to 86% width, converted to paths.
13. **LED labels.** Every LED is labelled `LEVEL`.
14. **Link lines** join a knob to the CV jack it scales wherever the jack sits directly under the knob.
15. **Layout checker.** `scripts/make_panels.py` fails if any component or label overlaps another, crosses a section outline, leaves the panel, breaks the spec's spacing rules, or lacks a label.

## Defaults

16. VCM section 1 `VC` (level) defaults to full, so LO mode passes the signal at unity. HI / LO defaults to LO.
17. RING `MOD` defaults to full ring modulation. `CARRIER` defaults to centre (clean carrier).
18. SSG `VC` knobs default to half.
19. COM comparators have 10 mV of hysteresis so a slow crossing gives one clean edge.

## Tests and build

20. Tests use a small built-in harness (`tests/check.hpp`) instead of doctest, so `make test` needs nothing downloaded.
21. CI builds Linux x64, Windows x64, macOS arm64 and macOS x64 (cross-compiled on an Apple silicon runner with `clang -arch x86_64`). The Linux job also fails if the plugin has unresolved symbols of its own.

## Not verified

22. No schematic was reachable as text, so none of the estimated constants in §12 of the spec could be checked against a circuit. They stand as specified, apart from items 1 and 2 above.

## Round two (WAD, FRS, BDR, PDIV)

Spec: `docs/BUILD-INSTRUCTIONS-ROUND-2.md`.

23. **BDR jacks.** A Rack port is one-way, so each router section has its 1-to-2 jacks and its 2-to-1 jacks side by side, both steered by the same SELECT (spec §3.2).
24. **BDR and WAD output limits.** The band-limiting correction at a switching instant (BDR) or a bucket-brigade step (WAD) can overshoot by up to half a jump. Those outputs pass through the rail clip so they stay within ±10.5 V.
25. **BDR band-limiting** gives 30 dB less aliasing than plain switching when SELECT is a smooth signal crossing its thresholds. With a SELECT that jumps in one sample (a pulse from another module) the crossing time is unknown inside the sample and the gain is smaller.
26. **PDIV width.** Seven outputs with labels do not fit in one column of 4 HP under the spacing rules, so PDIV is 6 HP with the outputs in two columns.
27. **WAD speed.** The line's soft limit is a rational fit of `6·tanh(v/6)` (within 2%), and its hiss is uniform rather than Gaussian (same 1 mV RMS), so the delay costs about 0.6% of a core even at its 1 MHz shortest-delay clock.
28. **FRS oscillator.** Computed exactly once per host sample and rotated between oversampled sub-samples.
29. **FRS aliasing test.** The spec's case (15 kHz shifted up 5 kHz) puts the result at 20 kHz, in the oversampler's transition band, and never aliases at the host rate. The test instead shifts 18 kHz up 8 kHz to 26 kHz and checks that nothing folds back below 20 kHz.
30. **WAD tests.** The filtered-aliasing test uses a 400 Hz filter setting rather than 500 Hz to clear −40 dB with margin. The Karplus-Strong pitch is measured by autocorrelation, since zero crossings follow the bright upper partials; it rings at 217.4 Hz for a nominal 220 Hz (1.2% flat, from the filters and the one-sample loop).
31. **Not verified.** As in round one, no schematic was readable as text. The FRS control list and every constant in spec §8's estimates list stand as specified.
