# Loom
An alternative firmware for the [Yarns MIDI interface by Mutable Instruments](https://mutable-instruments.net/modules/yarns/), with an emphasis on dynamic voicing, open-ended musical composition, and flexible control.

### Features
- [Looper-style sequencer with real-time recording and overdubbing](yarns/MANUAL.md#looper-style-sequencing-mode-with-real-time-recording)
- [10 new Braids-derived oscillator waveforms with timbre modulation](yarns/MANUAL.md#oscillator-controls)
- [ADSR envelope generator with velocity-sensitive segments and amplitude](yarns/MANUAL.md#adsr-envelopes-modulated-by-velocity)
- [New layouts, including a layout that features a 3-voice paraphonic part](yarns/MANUAL.md#layouts)
- [New and improved algorithms for polyphonic voicing](yarns/MANUAL.md#voicing-allocation-methods)
- [Global control and display of the active part and its play mode](yarns/MANUAL.md#global-control-and-display-of-the-active-part-and-its-play-mode)
- [Sequencer-driven arpeggiator](yarns/MANUAL.md#sequencer-driven-arpeggiator)
- [Velocity filtering](yarns/MANUAL.md#event-routing-filtering-and-transformation)
- [More control over input octave, sustain behavior, and sequencer/keyboard interaction](yarns/MANUAL.md#event-routing-filtering-and-transformation)
- [New ways to use the hold pedal, including latch, sostenuto, and note routing](yarns/MANUAL.md#hold-pedal)
- [Expanded CC support, including start/stop/delete recording](yarns/MANUAL.md#expanded-support-for-control-change-events)
- [Check the manual for more!](yarns/MANUAL.md)

### Caveats
- Installation of this firmware is at your own risk
- Presets saved in this firmware cannot be loaded with the manufacturer's firmware, and vice versa.  Users are advised to run `INIT` from the main menu after switching firmware
- Some changes are not documented in the changelog
- Some of Yarns' stock capabilities have been downgraded to accomodate new features (e.g. the sequencer holds 30 notes instead of the original 64)

### Installation
1. Download `yarns.syx` from the [latest release's assets](https://github.com/rcrogers/mutable-instruments-eurorack/releases/latest)
2. [Follow the manufacturer's instructions for installing new firmware](https://mutable-instruments.net/modules/yarns/manual/#firmware)

### Contributions and support
- License: MIT License
- Forks, pull requests, feature ideas, and bug reports are welcome
- Responses and merges are at my discretion
- [Patreon link](https://www.patreon.com/rcrogers)

### Acknowledgements
- Thanks to [forum user `Airell`](https://forum.mutable-instruments.net/t/yarns-firmware-wish-list/8051/39) for the idea of per-part latching
- Thanks to [forum user `sdejesus13`](https://forum.mutable-instruments.net/t/yarns-firmware-wish-list/8051/24) for encouraging the exploration of clock-based recording
- Thanks to [forum user `bloc`](https://forum.mutable-instruments.net/t/loom-alternative-firmware-for-yarns-looper-paraphony-and-more/17723/3) for beta testing, bug reports, support, and many great ideas
- And above all, thanks to Émilie Gillet for making a great module and then open-sourcing it!
