# Loom
An alternative firmware for the [Yarns MIDI interface by Mutable Instruments](https://mutable-instruments.net/modules/yarns/).

### Objectives
- Enhance Yarns as a tool for solo and collaborative musical composition
- Obtain maximal functionality from a minimal MIDI controller
- Expand Yarns' abilities as a digital synthesis voice

### Features
- New layouts, including a layout that features a 3-voice paraphonic part
- Global control and display of the active part and its play mode
- Looper with real-time recording and overdubbing
- Sequencer-driven arpeggiator
- Internal envelopes, with velocity-modulated segments and amplitude
- New wave shapes with timbral modulation for Yarns' digital oscillator
- Velocity filtering
- More control over vibrato, input octave, sustain behavior, and sequencer/keyboard interaction
- New ways to use the hold pedal, including latch and sostenuto
- Expanded CC support, including start/stop/delete recording
- [Check the manual for more!](yarns/MANUAL.md)

### Caveats
- Installation of this firmware is at your own risk
- Presets saved in this firmware cannot be loaded with the manufacturer's firmware, and vice versa
- Some changes are not documented in the changelog
- Some of Yarns' stock capabilities have been downgraded to accomodate new features (e.g. the sequencer holds 16 notes instead of the original 64)

### Installation
1. Download `yarns.syx` from the [latest release's assets](https://github.com/rcrogers/mutable-instruments-eurorack/releases/latest)
2. [Follow the manufacturer's instructions for installing new firmware](https://mutable-instruments.net/modules/yarns/manual/#firmware)

### Contributing
- License: MIT License
- Forks, pull requests, feature ideas, and bug reports are welcome
- Responses and merges are at my discretion

### Acknowledgements
- Thanks to [forum user `Airell`](https://forum.mutable-instruments.net/t/yarns-firmware-wish-list/8051/39) for the idea of per-part latching
- Thanks to [forum user `sdejesus13`](https://forum.mutable-instruments.net/t/yarns-firmware-wish-list/8051/24) for encouraging the exploration of clock-based recording
- Thanks to [forum user `bloc`](https://forum.mutable-instruments.net/t/loom-alternative-firmware-for-yarns-looper-paraphony-and-more/17723/3) for beta testing, bug reports, support, and many great ideas
- And above all, thanks to Émilie Gillet for making a great module and then open-sourcing it!
