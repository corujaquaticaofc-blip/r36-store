# Community game catalog

The R36 STORE imports a community index generated from the official
[PortsMaster/PortMaster-Info](https://github.com/PortsMaster/PortMaster-Info)
catalog.

Snapshot: 2026-11-07.

- 1,206 entries advertise R36S availability and an ARM64/ARMHF build.
- 574 are marked upstream as ready-to-run.
- The remaining entries may require original game files, demos, runtimes, or other setup.
- Every imported entry is labelled as untested by R36 STORE.

## Important installation status

This first integration is a **catalog and discovery layer**, not a claim that all
1,206 packages can already be installed by R36 STORE. PortMaster packages have
their own launch scripts, runtime requirements, and installation assumptions.
The store currently lists them for review; a separate compatibility/installer
adapter is required before enabling one-click installation. The existing
R36 STORE package installer remains restricted to its validated private package
format and does not execute arbitrary community shell scripts.

The catalog is filtered for the R36S device and supported ARM architectures.
The upstream source remains the authority for package availability, requirements,
and download URLs.
