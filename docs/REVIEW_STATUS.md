# Review status

## Completed

- Exact clean regeneration on a different platform/toolchain.
- Independent verification of all 4,208,748 graph/coloring records.
- Full nauty and Traces duplicate audit at every order through 17.
- Small-order comparison with independent NetworkX/VF2 generation.
- Author of the 18-vertex construction reviewed the earlier report and
  verifier and found no error, while requesting stronger canonicalization
  evidence; that evidence is now included.

## Still appropriate before formal publication

- Send the new nauty/Traces audit and this technical note to József Pintér.
- Ask one additional graph-theory/computational-combinatorics specialist to
  review the removable-vertex completeness proof and run the release gate.
- Publish the code, compact source archive, technical note, hashes, and a
  practical route to the larger certificate corpus in a stable repository.
- If desired, prepare an arXiv note or joint revision after expert feedback.

## Claim boundary

It is accurate now to say “reproducible computational proof” and “resolved an
open minimum-order question.” It is premature to say “peer reviewed,” “formally
verified,” or that the separate 2-connected minimum problem has been solved.
