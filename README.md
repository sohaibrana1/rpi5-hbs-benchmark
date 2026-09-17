# Raspberry Pi 5 Hash-Based Signature Benchmark

Reproducibility materials for the manuscript:

**Stateful and Stateless Hash-Based Signatures on Raspberry Pi 5:
An Empirical Study of XMSS, XMSS-MT, and SLH-DSA**

Author: Sohaib Rana  
Affiliation: Universiti Sains Malaysia, Penang, Malaysia

## Study scope

The study contains two separate experimental blocks.

### Stateful block

24 XMSS/XMSS-MT configurations:

- XMSS h = 10
- XMSS h = 16
- XMSS-MT H/d = 20/2
- XMSS-MT H/d = 20/4
- REF and FAST/BDS traversal implementations
- WOTS+ w = 4, 16, and 256

The stateful block is a 4 x 3 x 2 experimental design.

### Stateless block

All 12 standardized FIPS 205 SLH-DSA parameter sets:

- SHA2 and SHAKE
- security categories 128, 192, and 256
- small-signature (s) and fast-signing (f) profiles

The stateful and stateless blocks are analyzed separately and do not
constitute one unified 36-cell factorial experiment.

## Platform

Measurements were collected on:

- Raspberry Pi 5 Model B Rev. 1.1
- Broadcom BCM2712
- quad-core Arm Cortex-A76
- 8 GB RAM
- Debian GNU/Linux 13 (trixie)
- Linux kernel 6.18.34+rpt-rpi-2712
- GCC 14.2.0
- performance CPU-frequency governor
- benchmark process pinned to CPU 3

## Repository structure

- `analysis/` — publication-level analysis datasets
- `build_metadata/` — source-provenance and freeze-validation records
- `figures/` — publication figures
- `manifests/` — SHA-256 and publication freeze manifests
- `source_stateful/` — frozen XMSS/XMSS-MT source worktrees
- `source_slhdsa/` — frozen upstream SLH-DSA source snapshot
- `docs/` — reproducibility documentation

## Source provenance

### XMSS/XMSS-MT

Four authoritative source worktrees are included:

- w = 4
- w = 16
- w = 256
- experimental WOTS source

Each contains 41 files and was independently validated against its
SHA-256 source manifest with zero missing files and zero hash mismatches.

The original CC0 1.0 Universal license is retained in every stateful
source directory.

### SLH-DSA

Upstream implementation:

`pq-code-package/slhdsa-c`

Upstream repository:

https://github.com/pq-code-package/slhdsa-c

Frozen commit:

`174c02e42257f95c210963272877c49dbb50070f`

The copied source tree was verified against the Git tree of this exact
commit and matched exactly.

The upstream implementation is not original work by Sohaib Rana.
Original copyright and license terms remain applicable.

## Experimental integrity

This repository is a derived reproducibility copy created from frozen
benchmark artifacts.

The authoritative benchmark and source directories were not modified
during preparation of this repository.

## Licensing

This repository contains material under multiple licenses.

See `LICENSES.md`.

## Citation

Citation metadata are provided in `CITATION.cff`.

Archived release:

- Version: `v1.0.0`
- DOI: `10.5281/zenodo.22775812`
- DOI URL: https://doi.org/10.5281/zenodo.22775812

## Data archive

The frozen `v1.0.0` reproducibility release is permanently archived in
Zenodo:

https://doi.org/10.5281/zenodo.22775812

## Sequential-index extension release

Version `v1.1.0` adds the sequential-index BDS traversal extension for
the four standardized `w=16` FAST configurations:

- XMSS-SHA2_10_256
- XMSS-SHA2_16_256
- XMSSMT-SHA2_20/2_256
- XMSSMT-SHA2_20/4_256

Three independent measurement keys were used for each configuration,
giving 12 publication runs. Signature indices advance contiguously from
index 0 without reset; warm-up uses separate disposable key material.

The extension release includes:

- 12 authoritative per-signature CSV datasets;
- frozen source provenance and the sequential benchmark harness;
- run metadata and publication run matrix;
- descriptive, boundary-window, and index-aligned analysis datasets;
- five publication figures;
- validation and SHA-256 integrity records.

Release package:

`releases/v1.1.0/PAPER1_SEQUENTIAL_EXTENSION_RELEASE_v1.1.0.tar.gz`

Archive SHA-256:

`1c3d7e9b4095e8c9ca603afb7225881b160187008aec25537fd92441fad9df6a`

A Zenodo DOI for `v1.1.0` will be added after archival publication.
