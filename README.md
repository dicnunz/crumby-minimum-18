# Crumby Coloring

This repository contains a reproducible computational proof of the following
result:

> **The minimum number of vertices in a connected simple subcubic
> K4-minor-free graph without a crumby coloring is 18.**

József Pintér constructed the 18-vertex counterexample and proved that it has
the required properties. His 2026 paper explicitly left open whether 18 was
the minimum. The computation here establishes the matching lower bound by
checking every eligible graph through 17 vertices.

## Evidence at a glance

- **4,208,748** isomorphism classes checked through order 17.
- A directly verifiable crumby-coloring witness for every graph.
- A clean regeneration on a different platform reproduced all 51 data hashes.
- An independent Python verifier passed all 4,208,748 graph/witness records.
- Both nauty and Traces independently found no isomorphic duplicates at any
  order, including all 2,917,955 order-17 graphs.

The full argument and scope are in [the technical note](docs/TECHNICAL_NOTE.md).
This is a reproducible computational proof; it is not yet a peer-reviewed
publication.

## Verify the released certificates

Requirements: Python 3, `zstd`, and nauty's `shortg` command. On macOS:

```bash
brew install nauty zstd
```

Download the complete 43 MB certificate archive from the
[v1.0.0 release](https://github.com/dicnunz/crumby-coloring/releases/tag/v1.0.0),
then run:

```bash
mkdir certs
tar --use-compress-program=unzstd -xf crumby_certificates.tar.zst -C certs
./verify_release.sh certs
```

The release check verifies every file hash, independently checks every graph
and coloring, and repeats the full nauty/Traces duplicate audit. A successful
run ends with `release-check=PASS`.

## Inspect an individual certificate

The [interactive example](https://dicnunz.github.io/demos/crumby/) shows the
last released order-17 graph and its coloring. Select vertices to try other
colorings, inspect rule violations, restore the witness, or download the record.
The example is a saved, checked certificate; it is not a live enumeration.

To create the same standalone HTML inspector for any released record:

```bash
python3 inspect_certificate.py certs/data --order 17 --index 2917955 --output certificate.html
python3 -m unittest -v test_inspect_certificate.py
```

The index is one-based. The inspector checks the selected record's adjacency
key, graph structure, and coloring before writing output. It reuses
`verify_independent.py`; it is an interface to that verifier, not an additional
independent verifier. A single record does not certify the complete enumeration.

## Regenerate everything

The complete deterministic generation starts from the one-vertex graph:

```bash
THREADS=16 CHUNK_SIZE=100000 ./run_all.sh ./reproduction
```

This is substantially more expensive than checking the released certificates.
The generator requires a C++20 compiler with OpenMP.

## Repository map

- `src/`: exhaustive generator, canonicalizer, coloring search, and packaged
  verifier.
- `verify_independent.py`: separately written streaming certificate verifier.
- `audit/`: independent small-order generation, canonicalization, structural,
  graph6, nauty, and Traces checks.
- `reference/`: expected counts and SHA-256 manifests.
- `docs/TECHNICAL_NOTE.md`: result, proof structure, evidence, and limitations.
- `docs/REVIEW_STATUS.md`: completed review and remaining publication steps.

## Credit

- 18-vertex construction and hand proof: **József Pintér**.
- Exhaustive minimality computation, verification, and release package:
  **Nicholas Dunzelman with GPT-5.6 Pro / ChatGPT**.

## References

- J. Pintér, [*Subcubic K4-minor-free graphs without crumby
  colorings*](https://arxiv.org/abs/2605.04706), arXiv:2605.04706 (2026).
- J. Barát, Z. L. Blázsik, and G. Damásdi, [*Crumby colorings—red-blue vertex
  partition of subcubic graphs regarding a conjecture of
  Thomassen*](https://arxiv.org/abs/2108.08118), Discrete Mathematics 346
  (2023) 113281.
