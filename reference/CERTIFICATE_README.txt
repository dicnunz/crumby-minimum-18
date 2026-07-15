This archive contains the complete line-aligned certificate set for the exhaustive computation through order 17.

For each n=1,...,17:
  data/keys_n<n>.txt       strictly increasing 48-hex-digit canonical adjacency keys
  data/graphs_n<n>.g6      the same graphs in graph6 format
  data/witnesses_n<n>.txt  8-hex-digit red-vertex masks

Line i in the three files for one n refers to the same graph/coloring.
The source package's src/verify_cert.cpp independently checks every line.

manifest.tsv gives the exact counts and generation diagnostics.
SHA256_KEYS.txt, SHA256_GRAPHS.txt, and SHA256_WITNESSES.txt hash every data file.
