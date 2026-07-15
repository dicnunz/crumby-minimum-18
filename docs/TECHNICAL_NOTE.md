# The minimum order of a subcubic K4-minor-free graph without a crumby coloring

Nicholas Dunzelman, with GPT-5.6 Pro / ChatGPT
Computational note, 15 July 2026

## Result

**Theorem.** The minimum number of vertices in a connected simple graph of
maximum degree at most three, with no K4 minor and no crumby coloring, is 18.

A crumby coloring is a red-blue vertex coloring in which the blue induced
subgraph has maximum degree at most one, while the red induced subgraph has no
isolated vertex and contains no simple path on four vertices.

József Pintér supplied the upper bound by constructing and proving correct an
18-vertex counterexample. His paper explicitly left open whether 18 was the
minimum possible order. The computation described here supplies the matching
lower bound.

## Exhaustive lower bound

Every connected simple subcubic K4-minor-free graph on at most 17 vertices was
generated up to isomorphism. The resulting corpus contains 4,208,748 classes:

| n | classes | n | classes |
|---:|---:|---:|---:|
| 1 | 1 | 10 | 1,028 |
| 2 | 1 | 11 | 2,936 |
| 3 | 2 | 12 | 8,926 |
| 4 | 5 | 13 | 27,262 |
| 5 | 9 | 14 | 86,010 |
| 6 | 23 | 15 | 274,129 |
| 7 | 50 | 16 | 889,919 |
| 8 | 138 | 17 | 2,917,955 |
| 9 | 354 | **total** | **4,208,748** |

Every graph has an explicit red-vertex mask. A separately written verifier
checked connectedness, the degree bound, treewidth at most two, key/graph
agreement, and all three crumby-coloring conditions for every record. No
uncolorable graph occurred through order 17.

## Why the generation is complete

K4-minor-free graphs are exactly the partial 2-trees. Every connected partial
2-tree with at least two vertices has a non-cut vertex of degree one or two.
This follows from a leaf block of the block-cut tree: a bridge supplies the
degree-one case; a 2-connected leaf block can be completed on the same vertex
set to a 2-tree, which has at least two degree-two vertices, at most one of
which is the attachment cut vertex.

Deleting that vertex preserves connectedness, subcubicity, and K4-minor
freeness. Reversing the deletion shows that every target graph occurs by
adding either a leaf or a degree-two vertex to a graph at the preceding order.
The generator performs precisely these augmentations, applies the exact
width-two elimination test, canonicalizes, and deduplicates. Induction from
the one-vertex graph therefore covers every eligible isomorphism class.

## Independent isomorphism audit

The custom canonicalizer is not the sole evidence for deduplication. Every
stored graph at every order was independently processed by both canonical
engines in nauty 2.9.3: nauty's default engine and Traces. At each order, both
reported exactly as many isomorphism classes as input records. In particular,
all 2,917,955 order-17 graphs remained distinct under both engines. The audit
is reproducible with `../audit/run_nauty_audit.sh`.

This closes the principal implementation risk identified in external review:
accidentally retaining two differently labeled copies of the same graph.

## Reproducibility and checks

- A clean ARM64/macOS regeneration with a different compiler reproduced all
  51 graph, key, and witness hashes exactly.
- The independent Python verifier passed all 4,208,748 records.
- nauty and Traces independently found no isomorphic duplicates at any order.
- NetworkX/VF2 generation reproduced the exact class counts through order 10.
- A second custom canonicalizer checked every graph through order 12 and a
  deterministic order-17 sample.
- The removable-vertex property was checked on every generated graph.

The order-17 graph corpus has SHA-256
`c84c5812ed8285494ab30ac29083bcb922aaf3739e8603a160acb53652f1b70c`.

## Scope and status

This is a reproducible computational proof, not a journal peer review. The
author of the 18-vertex construction reviewed the earlier report and
independent verifier favorably, while correctly identifying full independent
canonical identification as the remaining audit priority. The nauty/Traces
audit was completed afterward and has not yet received his follow-up review.

The separate problem of the minimum order under an additional 2-connectivity
requirement remains outside this result; Pintér's 40-vertex example gives an
upper bound for that problem.

## References

1. J. Pintér, *Subcubic K4-minor-free graphs without crumby colorings*,
   arXiv:2605.04706 (2026), https://arxiv.org/abs/2605.04706.
2. J. Barát, Z. L. Blázsik, and G. Damásdi, *Crumby colorings—red-blue vertex
   partition of subcubic graphs regarding a conjecture of Thomassen*, Discrete
   Mathematics 346 (2023) 113281, https://arxiv.org/abs/2108.08118.
