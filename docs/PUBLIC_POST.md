# Recommended public wording

## Short post

I used GPT-5.6 Pro / ChatGPT to help solve an open graph-theory question: is
18 the smallest possible connected subcubic K4-minor-free graph with no
crumby coloring?

The answer is yes. József Pintér had constructed the 18-vertex example and
explicitly left its minimality open. We exhaustively generated all 4,208,748
eligible isomorphism classes on at most 17 vertices, produced a valid coloring
certificate for every one, regenerated the corpus independently, and audited
all graphs with both nauty and Traces.

So the result is a reproducible computational proof that the minimum is 18.
The package is ready for specialist follow-up review; it is not yet a
peer-reviewed publication.

## One-sentence version

GPT-5.6 Pro / ChatGPT and I produced a reproducible computational proof that
18 is the minimum order of a connected subcubic K4-minor-free graph without a
crumby coloring, resolving a question explicitly left open in József Pintér's
2026 paper.

## Credit line

18-vertex construction and hand proof: József Pintér. Exhaustive minimality
computation, verification, and release package: Nicholas Dunzelman with
GPT-5.6 Pro / ChatGPT.
