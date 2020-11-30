# Loom
An alternate firmware for the [Yarns MIDI interface by Mutable Instruments](https://mutable-instruments.net/modules/yarns/).

### Objectives
- Enhance Yarns as a tool for solo and collaborative musical composition
- Obtain maximal functionality from a minimal MIDI controller
- Expand Yarns' abilities as a digital synthesis voice

### Features
- New layouts, including a paraphonic layout with internal envelopes
- Looper with real-time recording
- Global control and display of the active part and its play mode
- Oscillator PWM
- Velocity filtering
- More control over vibrato, input octave, sustain behavior, and sequencer/keyboard interaction
- Sequencer-driven arpeggiator
- [Check the changelog for more!](https://github.com/rcrogers/mutable-instruments-eurorack/releases)

### Caveats
- Installation of this firmware is at your own risk
- Presets saved in this firmware cannot be loaded with the manufacturer's firmware, and vice versa
- Some changes are not documented in the changelog
- Some of Yarns' stock capabilities have been downgraded to accomodate new features (e.g. the sequencer holds 16 notes instead of the original 64)

### Installation
1. Download `yarns.syx` from the [latest release's assets](https://github.com/rcrogers/mutable-instruments-eurorack/releases/latest)
2. [Follow the manufacturer's instructions for installing new firmware](https://mutable-instruments.net/modules/yarns/manual/#firmware)

### Contributing
Forks, pull requests, feature ideas, and bug reports are welcome.  Responses and merges are at my discretion.

### Acknowledgements
- Thanks to [forum user `Airell`](https://forum.mutable-instruments.net/t/yarns-firmware-wish-list/8051/39) for the idea of per-part latching
- Thanks to [forum user `sdejesus13`](https://forum.mutable-instruments.net/t/yarns-firmware-wish-list/8051/24) for encouraging the exploration of clock-based recording
- And above all, thanks to Émilie Gillet for making a great module and then open-sourcing it!
