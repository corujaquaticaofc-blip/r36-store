# Community game catalog

The R36 STORE imports a community index generated from the official
[PortsMaster/PortMaster-Info](https://github.com/PortsMaster/PortMaster-Info)
catalog.

Snapshot: 2026-11-07.

- 1,206 entries advertise R36S availability and an ARM64/ARMHF build.
- 574 are marked upstream as ready-to-run.
- The remaining entries may require original game files, demos, runtimes, or other setup.
- "Ready to run" is upstream metadata, not a compatibility test by R36 STORE.

## Community adapter profiles

The store now has two independent package adapters:

1. **RetroArch/libretro:** recognizes a package containing one ARM64 libretro core and an upstream launcher that invokes RetroArch.
2. **Native SDL + keymap:** recognizes a single ARM64 ELF executable, checks its dynamic libraries with `readelf`, and reads one upstream `.gptk` controller profile. R36 STORE generates its own launcher and uses its own Python/evdev-to-uinput key mapper. The mapper uses the legacy uinput ABI because this dArkOS device runs Linux 4.4.

The installer downloads only HTTPS packages from GitHub, checks the published archive size and MD5, rejects unsafe ZIP paths, symbolic links, duplicate entries, and oversized archives, then extracts into the private R36 STORE game directory.

The adapter does **not** execute upstream PortMaster shell launchers, source PortMaster control files, call PortMaster runtime helpers, or modify the PortMaster installation. It rejects packages that do not match a supported profile or that have unresolved shared-library dependencies.

## Adapter-ready candidates

Static package checks currently identify 36 candidates for installation through R36 STORE:

- 2048
- Wordle SDL
- SDL Sopwith
- Celeste Classic
- Falling Time
- Raptor
- Nonny
- OpenSyobon
- Shippy
- Der Clou!
- Meritous
- Worship Vector
- Escape
- Powder
- Znax
- ColorLines
- Mutant Tank Knights
- Nebulus
- OpenFodder
- GNU Robbo
- LBreakoutHD
- mReader
- Waternet
- xkcd Hoverboard
- iiChantra
- Ultimate Tapan Kaikki
- Cave Packer
- Abuse
- nKaruga
- Abu Simbel Profanation Deluxe
- vitaSnake
- Pekka Kana 2
- BlockOut II
- Demolition Dome
- Cave Story (Evo)
- Hex-A-Hop

These are **adapter-ready, not gameplay-verified**. Package download, MD5, safe extraction, architecture, dependency checks, and launcher generation were tested in temporary directories. No game was launched remotely. Each candidate still needs a physical control/gameplay test on the R36S before it can be labelled verified.

The community list remains a discovery catalog; it does not mean all 1,206 entries are installable.
