# Repository release rules

This repository publishes the computational minimum-order result for crumby
colorings. Preserve the reference hashes and counts. Do not claim publication
readiness unless `./verify_release.sh <certificate-directory>` passes against
the complete certificate corpus.

Public wording must distinguish a reproducible computational proof from formal
peer review. Credit József Pintér for the 18-vertex construction and hand proof,
and Nicholas Dunzelman with GPT-5.6 Pro/ChatGPT for the exhaustive minimality
computation and verification package.
