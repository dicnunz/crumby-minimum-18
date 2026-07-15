#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-certs/data}"
OUT="${2:-nauty_audit/results.tsv}"

for command in shortg awk wc; do
  command -v "$command" >/dev/null || {
    echo "missing required command: $command" >&2
    exit 2
  }
done

mkdir -p "$(dirname "$OUT")"
tmp="${OUT}.tmp"
printf 'n\tinput\tnauty_unique\ttraces_unique\tstatus\n' > "$tmp"

extract_count() {
  awk '/^>Z [0-9]+ graphs produced$/ { print $2 }'
}

for n in $(seq 1 17); do
  input="$ROOT/graphs_n${n}.g6"
  expected=$(awk 'END { print NR }' "$input")
  nauty=$(shortg -u "$input" 2>&1 | extract_count)
  traces=$(shortg -t -u "$input" 2>&1 | extract_count)
  status=PASS
  if [[ -z "$nauty" || -z "$traces" || "$nauty" != "$expected" || "$traces" != "$expected" ]]; then
    status=FAIL
  fi
  printf '%s\t%s\t%s\t%s\t%s\n' "$n" "$expected" "${nauty:-ERROR}" "${traces:-ERROR}" "$status" | tee -a "$tmp"
done

if awk -F '\t' 'NR > 1 && $5 != "PASS" { bad=1 } END { exit bad }' "$tmp"; then
  mv "$tmp" "$OUT"
  total=$(awk -F '\t' 'NR > 1 { sum += $2 } END { print sum }' "$OUT")
  echo "total=$total dual-canonical-audit=PASS"
else
  mv "$tmp" "$OUT"
  echo "dual-canonical-audit=FAIL" >&2
  exit 1
fi
