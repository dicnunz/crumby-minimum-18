#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
OUT=${1:-"$ROOT/reproduction"}
THREADS=${THREADS:-16}
CHUNK_SIZE=${CHUNK_SIZE:-100000}
BUCKETS=${BUCKETS:-256}
CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:-"-O3 -std=c++20 -march=native -DNDEBUG"}

mkdir -p "$OUT/bin" "$OUT/data" "$OUT/tmp" "$OUT/logs"

$CXX $CXXFLAGS -fopenmp "$ROOT/src/level_generate.cpp" -o "$OUT/bin/level_generate"
$CXX $CXXFLAGS "$ROOT/src/merge_sorted.cpp" -o "$OUT/bin/merge_sorted"
$CXX $CXXFLAGS -fopenmp "$ROOT/src/color_keys.cpp" -o "$OUT/bin/color_keys"
$CXX $CXXFLAGS "$ROOT/src/keys_to_g6.cpp" -o "$OUT/bin/keys_to_g6"
$CXX $CXXFLAGS "$ROOT/src/verify_cert.cpp" -o "$OUT/bin/verify_cert"

printf '%048d\n' 0 > "$OUT/data/keys_n1.txt"
printf 'n\tcount\traw_leaf\traw_pair\ttw2_reject\tduplicate\tcanon_leaves\tnext_count\n' > "$OUT/transitions.tsv"

for n in $(seq 1 16); do
    current="$OUT/data/keys_n${n}.txt"
    next="$OUT/data/keys_n$((n+1)).txt"
    count=$(awk 'END { print NR }' "$current")
    level_tmp="$OUT/tmp/level_${n}"
    rm -rf "$level_tmp"
    mkdir -p "$level_tmp"
    split -d -a 4 -l "$CHUNK_SIZE" "$current" "$level_tmp/in_"
    : > "$level_tmp/stats.tsv"
    for input in "$level_tmp"/in_*; do
        suffix=${input##*_}
        "$OUT/bin/level_generate" "$n" "$input" "$level_tmp/out_$suffix" "$THREADS" "$BUCKETS" \
            >> "$level_tmp/stats.tsv"
    done
    "$OUT/bin/merge_sorted" "$next" "$level_tmp"/out_* > "$level_tmp/merge.tsv"
    next_count=$(awk 'END { print NR }' "$next")
    read -r parents raw_leaf raw_pair reject accepted canon_leaves local_unique < <(
        awk -F '\t' '{p+=$1;l+=$2;q+=$3;r+=$4;a+=$5;c+=$6;u+=$7}
             END{print p,l,q,r,a,c,u}' "$level_tmp/stats.tsv"
    )
    if [[ "$parents" != "$count" ]]; then
        echo "parent-count mismatch at n=$n" >&2
        exit 3
    fi
    duplicate=$((accepted-next_count))
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$n" "$count" "$raw_leaf" "$raw_pair" "$reject" "$duplicate" "$canon_leaves" "$next_count" \
        >> "$OUT/transitions.tsv"
    rm -rf "$level_tmp"
done

count17=$(awk 'END { print NR }' "$OUT/data/keys_n17.txt")
printf '17\t%s\t0\t0\t0\t0\t0\t0\n' "$count17" >> "$OUT/transitions.tsv"

printf 'n\tcount\tcolorable\tuncolorable\n' > "$OUT/coloring.tsv"
for n in $(seq 1 17); do
    "$OUT/bin/color_keys" "$n" "$OUT/data/keys_n${n}.txt" \
        "$OUT/data/witnesses_n${n}.txt" "$THREADS" 1 >> "$OUT/coloring.tsv"
    "$OUT/bin/keys_to_g6" "$n" "$OUT/data/keys_n${n}.txt" \
        "$OUT/data/graphs_n${n}.g6" >> "$OUT/logs/g6_conversion.log"
    "$OUT/bin/verify_cert" "$n" "$OUT/data/keys_n${n}.txt" \
        "$OUT/data/graphs_n${n}.g6" "$OUT/data/witnesses_n${n}.txt" \
        >> "$OUT/logs/verification.log"
done

awk -F '\t' 'BEGIN{OFS="\t"}
    NR==FNR {if(FNR>1){color[$1]=$3; uncolor[$1]=$4} next}
    FNR==1 {print "n","count","raw_leaf","raw_pair","tw2_reject","duplicate","canon_leaves","colorable","uncolorable"; next}
    {print $1,$2,$3,$4,$5,$6,$7,color[$1],uncolor[$1]}' \
    "$OUT/coloring.tsv" "$OUT/transitions.tsv" > "$OUT/manifest.tsv"

if command -v sha256sum >/dev/null; then
    HASH_COMMAND=(sha256sum)
else
    HASH_COMMAND=(shasum -a 256)
fi
(
    cd "$OUT/data"
    "${HASH_COMMAND[@]}" keys_n*.txt > "$OUT/SHA256_KEYS.txt"
    "${HASH_COMMAND[@]}" graphs_n*.g6 > "$OUT/SHA256_GRAPHS.txt"
    "${HASH_COMMAND[@]}" witnesses_n*.txt > "$OUT/SHA256_WITNESSES.txt"
)
{
    "$CXX" --version | head -1
    uname -a
    echo "threads=$THREADS"
    echo "chunk_size=$CHUNK_SIZE"
    echo "buckets=$BUCKETS"
} > "$OUT/environment.txt"

echo "Completed. Results are in $OUT"
