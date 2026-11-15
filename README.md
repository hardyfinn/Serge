# Serge

A Serge Modular system for VCV Rack 2: 23 modules modelled on the classic Serge circuits (Serge Tcherepnin's designs of the 1970s and 80s), with paperface-style panels and the Serge banana-jack colour code.

| | |
|---|---|
| Slopes and random | DSG Dual Universal Slope Generator, SSG Smooth & Stepped Generator, NOI Noise Source, ASR Analog Shift Register |
| Voice | NTO New Timbral Oscillator, VCM Wave Multipliers, TWS Triple Waveshaper, RING Ring Modulator |
| Filters | VCFQ Variable Q VCF, VCFS Variable Slope VCF, RESEQ Resonant Equalizer |
| Delay and shifting | WAD Wilson Analog Delay, FRS Frequency Shifter |
| Logic and utility | COM Dual Comparator, STR Dual Schmitt Trigger, BDR Bi-Directional Router, PDIV Pulse Divider, PRC Dual Processor, XFAD Cross-Fader, VCA2 Dual VCA, MIX Dual Audio Mixer, DCSM Dual Channel Stereo Mixer, PT Peak & Trough |

**Levels are Serge levels, not Rack's.** Audio is about 5 V peak to peak, control voltages run 0 to 5 V, and pulses are 0 or 5 V. Black jacks carry audio, blue jacks control voltages, red jacks pulses, and yellow jacks the rail-swinging SSG outputs. See [docs/MANUAL.md](docs/MANUAL.md).

## Install

1. Open the latest run of the **Build** workflow under the repository's Actions tab, or the newest release.
2. Download the file for your computer: `Serge-win-x64`, `Serge-mac-arm64` (Apple silicon), `Serge-mac-x64` (Intel Mac) or `Serge-lin-x64`. Unzip it to get a `.vcvplugin` file.
3. Put the `.vcvplugin` file in Rack's plugins folder (in Rack: Help → Open user folder, then the `plugins-<os>-<cpu>` folder) and restart Rack. Rack unpacks it on start.

## Build from source

```
export RACK_DIR=/path/to/Rack-SDK
make && make install      # or: make dist
make test                 # model tests, no SDK needed
make panels               # regenerate panels (Python 3 + fonttools)
```

## Documents

- [docs/MANUAL.md](docs/MANUAL.md): how each module works, with patches to try.
- [docs/BUILD-INSTRUCTIONS.md](docs/BUILD-INSTRUCTIONS.md) and [docs/BUILD-INSTRUCTIONS-ROUND-2.md](docs/BUILD-INSTRUCTIONS-ROUND-2.md): the design specifications.
- [docs/Serge-Manual.pdf](docs/Serge-Manual.pdf): the manual as a PDF (`scripts/make_manual_pdf.sh` rebuilds it).
- [docs/DECISIONS.md](docs/DECISIONS.md): where the build departs from or fills in the specification.
- [docs/CREDITS.md](docs/CREDITS.md)

Licence: GPL-3.0-or-later.
