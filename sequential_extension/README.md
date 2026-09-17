# Paper 1 Sequential-Index BDS Extension

This package contains the publication evidence for the sequential-index
extension of the Raspberry Pi 5 hash-based-signature benchmark study.

## Scope

The extension evaluates the four standardized w=16 FAST configurations:

- XMSS-SHA2_10_256
- XMSS-SHA2_16_256
- XMSSMT-SHA2_20/2_256
- XMSSMT-SHA2_20/4_256

Three independently generated measurement keys were used for each
configuration, giving 12 publication runs.

Each measurement key begins at signature index 0 and advances
contiguously without reset. Warm-up uses separate disposable key
material and is excluded from the publication observations.

## Directory structure

01_raw_data/
    Twelve authoritative per-signature publication CSV files.

02_source/frozen/
    Frozen w=16 source archive and its SHA-256 source manifest.

02_source/harness/
    Sequential-index measurement harness.

03_metadata/
    Source provenance, frozen-source record, publication run matrix,
    publication run plan, dataset manifest, and the 12 per-run metadata
    records.

04_analysis/
    Publication analysis datasets and scripts used to generate the
    sequential-extension figures and supplementary tables.

05_figures/
    Five publication figures for the sequential-index extension.

06_validation/
    Publication-data, analysis, figure, supplementary-table, and
    final-freeze validation records.

## Experimental environment

Measurements were performed on Raspberry Pi 5 and pinned to CPU 3
under the performance governor.

The extension binaries were compiled with GCC 14.2.0 using:

    -std=gnu11 -O3 -Wall -Wextra -Wpedantic -DNDEBUG

The stateful implementation uses the OpenSSL 3.5.7 libcrypto SHA-256
back end.

Recorded pre-run and post-run firmware throttling checks reported
throttled=0x0 for all 12 publication runs.

## Integrity

SHA256_MANIFEST.txt contains SHA-256 hashes for every release file
except the manifest itself.

The original benchmark and validation artifacts remain preserved in
the provenance-controlled research workspace; this release package is
a publication-oriented copy and does not modify those originals.
