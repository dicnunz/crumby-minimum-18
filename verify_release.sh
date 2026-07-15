#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
CERT_ROOT=${1:-"$ROOT/certs"}

for command in python3 shasum shortg; do
  command -v "$command" >/dev/null || {
    echo "missing required command: $command" >&2
    exit 2
  }
done

test -d "$CERT_ROOT/data" || {
  echo "certificate data directory not found: $CERT_ROOT/data" >&2
  exit 2
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "[1/3] Reference hashes"
for manifest in SHA256_KEYS.txt SHA256_GRAPHS.txt SHA256_WITNESSES.txt; do
  (cd "$CERT_ROOT/data" && shasum -a 256 -c "$ROOT/reference/$manifest")
done

echo "[2/3] Independent graph and coloring verification"
python3 "$ROOT/verify_independent.py" "$CERT_ROOT/data"

echo "[3/3] Independent nauty and Traces isomorphism audit"
"$ROOT/audit/run_nauty_audit.sh" "$CERT_ROOT/data" "$tmp/nauty_results.tsv"
cmp "$ROOT/audit/nauty_results.tsv" "$tmp/nauty_results.tsv"

echo "release-check=PASS"
