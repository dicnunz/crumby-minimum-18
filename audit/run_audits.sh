#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${1:-"$ROOT/results/certificates"}
BUILD=${2:-"$ROOT/audit/build"}
mkdir -p "$BUILD"

python "$ROOT/audit/atlas_check.py"
python "$ROOT/audit/networkx_generation_check.py"
python "$ROOT/audit/graph6_networkx_audit.py" "$DATA"

g++ -O3 -std=c++20 -march=native -DNDEBUG \
    "$ROOT/audit/slow_canon_audit.cpp" -o "$BUILD/slow_canon_audit"
for n in $(seq 1 12); do
    "$BUILD/slow_canon_audit" "$DATA/keys_n${n}.txt" "$n" 1
done
"$BUILD/slow_canon_audit" "$DATA/keys_n17.txt" 17 293

g++ -O3 -std=c++20 -march=native -DNDEBUG \
    "$ROOT/audit/removable_vertex_audit.cpp" -o "$BUILD/removable_vertex_audit"
for n in $(seq 1 17); do
    "$BUILD/removable_vertex_audit" "$n" "$DATA/keys_n${n}.txt"
done
