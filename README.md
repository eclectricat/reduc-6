# Reduc-6: POC lowcost 'groovebox' device

Source code and hardware specs of a proof-of-concept minimal standalone sound machine.

Originally designed as a minimal low-cost hardware device, but can also be built for desktop or iOS (plugin, or standalone app).

The goal is to explore minimal hardware UI for sound creation, as a sandbox with contrained interaction surface yet infinite possibilities.


## Features

- self contained sound machine using Teensy microcontroller
- 6 voice modular sound engine, multitimbral or polyphonic mode (or mixed)
- Two types of voices: VA synth, or percussion oriented
- 6 sequencer tracks, up to 64 steps (configurable per track for polymetric sequenes)
- All sound parameters can be parameter locked per step
- Some global or per-track effects (delay, bit reduction, sample rate reduction, overdrive, stutter/beat-repeat)
- Per track probability
- USB Midi and USB Audio out, Stereo or 8-track (Stereo mix and 6 individual tracks, for integration into DAW setup)

## Hardware

- teensy 4.1
- Teensy Audio board
- 10 Tactile switches
- 4 Potentiometers
- 16x2 lcd display

## Desktop / iOS build

Alternative builds as AU plugin were added to facilitate seamless integration into a DAW, 
and as a tool during development, for debugging, benchmarking and faster development iterations. They are based on the iPlug2 framework.
The sound engine is platform-independent
The hardware abstraction layer encompasses display, knobs and buttons, audio interface, midi interface, and system functionality such as logging, timers etc.

The iPlug2 UI was on purpose kept minimal and identical to the interface on the teensy. Of course, UI improvements and advanced features would be low hanging fruits 
(e.g. better browser for stored projects and patches, real-time audio waveform display, graphical display, recording of audio performances)

Building the code using the xcode project should be straightforward on MacOS.

