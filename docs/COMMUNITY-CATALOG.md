# Community game catalog

The R36 STORE imports a community index generated from the official
[PortsMaster/PortMaster-Info](https://github.com/PortsMaster/PortMaster-Info)
catalog.

Snapshot: 2026-11-07.

- 1,206 entries advertise R36S availability and an ARM64/ARMHF build.
- 574 are marked upstream as ready-to-run.
- The remaining entries may require original game files, demos, runtimes, or other setup.
- Every imported entry is labelled as untested by R36 STORE.

## Community adapter status

The first adapter is now integrated into the store's install flow. It currently
supports a deliberately narrow profile: ready-to-run PortMaster packages whose
official launcher identifies a RetroArch/libretro core and whose archive
contains an ARM64 core.

For this profile, R36 STORE:

1. Downloads the official package URL from the community index.
2. Checks the published archive size and MD5 before extraction.
3. Rejects unsafe ZIP paths, symbolic links, duplicate entries, and oversized archives.
4. Extracts into the private R36 STORE game directory.
5. Generates its own launcher for the ARM64 libretro core.
6. Registers the game in the R36 STORE library.

The adapter does **not** execute upstream PortMaster shell launchers, source
PortMaster control files, or modify the PortMaster installation. A package
that needs PortMaster runtime functions, external game files, additional
libraries, or a launcher format outside the supported profile is rejected
without installation.

## Important installation status

The 1,206 entries are a discovery catalog, not 1,206 confirmed installable
games. "Ready to run" is upstream metadata, not an R36 STORE compatibility
test. Each supported package still needs a physical test on the R36S before
it can be labelled verified.
