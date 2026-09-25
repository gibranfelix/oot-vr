# Domain context

This file defines the terms that this project uses. It does not tell how the
code works. Read [`docs/architecture.md`](docs/architecture.md) for that. The
decisions that are difficult to change are in [`docs/adr/`](docs/adr/).

## Destination

The state in which this project is complete. The roadmap issue
([#1](https://github.com/oot-vr/oot-vr/issues/1)) contains it. The destination is not a task list and not a date.
It describes what success is.

## Standalone

The game runs natively on the headset, without a PC. This is the main
requirement of the destination. Other VR ports of Ocarina of Time need a PC.
Read [`docs/adr/0001`](docs/adr/0001-standalone-not-pcvr.md).

## PCVR

The game runs on a PC. The headset is a display that connects with a cable or
with Air Link. In this project, PCVR is not a destination. It is only a test
bench.

## Practical floor

The quality level of the destination. The player must be able to get to the end
of the game. Crashes are acceptable when the player can save frequently, load
again, and go around the problem. A "no crashes" level is not possible to
measure in a game of 25 to 30 hours.

## Main line

The scope of the destination: the Deku Tree, Dodongo's Cavern, Jabu-Jabu's
Belly, the five adult temples, and Ganon. Optional content is not part of the
destination: Gold Skulltulas, masks, and precision minigames. If a minigame does
not work in VR, the project did not fail.

## VR comfort

The player must not feel sick. VR comfort is part of the destination. The
target is sessions of one hour without discomfort.

## Port assets and game assets

Two different sets of files. They go to the headset in different ways.

- `soh.o2r` contains the **port** assets. The build makes this file, and the
  APK contains it.
- `oot.o2r` contains the **game** assets. The player makes this file from a dump
  of a cartridge that the player owns.

This repository never contains game assets.
