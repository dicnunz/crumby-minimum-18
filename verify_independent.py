#!/usr/bin/env python3
"""Independent streaming verifier for the crumby-coloring certificate set.

This implementation does not import or call any code from the submitted package.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def parse_graph6(line: bytes, expected_n: int) -> list[int]:
    raw = line.rstrip(b"\n\r")
    if not raw or raw[0] < 63 or raw[0] > 125:
        raise ValueError("unsupported graph6 header")
    n = raw[0] - 63
    if n != expected_n:
        raise ValueError(f"graph6 order {n}, expected {expected_n}")
    bit_count = n * (n - 1) // 2
    if len(raw) != 1 + (bit_count + 5) // 6:
        raise ValueError("graph6 length mismatch")
    adj = [0] * n
    bit = 0
    for high in range(1, n):
        for low in range(high):
            value = raw[1 + bit // 6] - 63
            if value < 0 or value > 63:
                raise ValueError("invalid graph6 byte")
            if value & (1 << (5 - bit % 6)):
                adj[low] |= 1 << high
                adj[high] |= 1 << low
            bit += 1
    return adj


def key_words(line: bytes) -> tuple[int, int, int]:
    raw = line.rstrip(b"\n\r")
    if len(raw) != 48:
        raise ValueError("key is not 48 hexadecimal digits")
    return tuple(int(raw[i : i + 16], 16) for i in (0, 16, 32))  # type: ignore[return-value]


def adjacency_words(adj: list[int]) -> tuple[int, int, int]:
    words = [0, 0, 0]
    bit = 0
    for low in range(len(adj)):
        for high in range(low + 1, len(adj)):
            if adj[low] & (1 << high):
                words[bit // 64] |= 1 << (bit % 64)
            bit += 1
    return words[0], words[1], words[2]


def is_connected(adj: list[int]) -> bool:
    seen = 1
    frontier = 1
    while frontier:
        neighbors = 0
        todo = frontier
        while todo:
            bit = todo & -todo
            neighbors |= adj[bit.bit_length() - 1]
            todo ^= bit
        frontier = neighbors & ~seen
        seen |= frontier
    return seen.bit_count() == len(adj)


def is_partial_2_tree(adj: list[int]) -> bool:
    work = adj.copy()
    active = (1 << len(adj)) - 1
    while active:
        chosen = -1
        todo = active
        while todo:
            bit = todo & -todo
            vertex = bit.bit_length() - 1
            if (work[vertex] & active).bit_count() <= 2:
                chosen = vertex
                break
            todo ^= bit
        if chosen < 0:
            return False
        neighbors = work[chosen] & active
        if neighbors.bit_count() == 2:
            first_bit = neighbors & -neighbors
            second_bit = neighbors ^ first_bit
            first = first_bit.bit_length() - 1
            second = second_bit.bit_length() - 1
            work[first] |= second_bit
            work[second] |= first_bit
        active &= ~(1 << chosen)
    return True


def is_crumby(adj: list[int], red: int) -> bool:
    full = (1 << len(adj)) - 1
    if red & ~full:
        return False
    blue = full ^ red
    todo = blue
    while todo:
        bit = todo & -todo
        vertex = bit.bit_length() - 1
        if (adj[vertex] & blue).bit_count() > 1:
            return False
        todo ^= bit
    todo = red
    while todo:
        bit = todo & -todo
        vertex = bit.bit_length() - 1
        if not (adj[vertex] & red):
            return False
        todo ^= bit
    # Enumerate every oriented three-edge walk on distinct red vertices.
    for a in range(len(adj)):
        if not (red & (1 << a)):
            continue
        bs = adj[a] & red
        while bs:
            bbit = bs & -bs
            b = bbit.bit_length() - 1
            cs = adj[b] & red & ~(1 << a)
            while cs:
                cbit = cs & -cs
                c = cbit.bit_length() - 1
                if adj[c] & red & ~((1 << a) | (1 << b)):
                    return False
                cs ^= cbit
            bs ^= bbit
    return True


def verify_order(root: Path, n: int) -> int:
    paths = [
        root / f"keys_n{n}.txt",
        root / f"graphs_n{n}.g6",
        root / f"witnesses_n{n}.txt",
    ]
    count = 0
    previous: tuple[int, int, int] | None = None
    with paths[0].open("rb") as keys, paths[1].open("rb") as graphs, paths[2].open("rb") as witnesses:
        while True:
            key_line, graph_line, witness_line = keys.readline(), graphs.readline(), witnesses.readline()
            if not (key_line or graph_line or witness_line):
                break
            if not (key_line and graph_line and witness_line):
                raise AssertionError(f"n={n} line-count mismatch after {count}")
            key = key_words(key_line)
            if previous is not None and not previous < key:
                raise AssertionError(f"n={n} keys not strictly increasing at {count}")
            adj = parse_graph6(graph_line, n)
            if adjacency_words(adj) != key:
                raise AssertionError(f"n={n} key/graph mismatch at {count}")
            if any(row.bit_count() > 3 for row in adj):
                raise AssertionError(f"n={n} non-subcubic graph at {count}")
            if not is_connected(adj):
                raise AssertionError(f"n={n} disconnected graph at {count}")
            if not is_partial_2_tree(adj):
                raise AssertionError(f"n={n} graph has treewidth >2 at {count}")
            red = int(witness_line, 16)
            if not is_crumby(adj, red):
                raise AssertionError(f"n={n} invalid coloring at {count}")
            previous = key
            count += 1
    return count


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("data", type=Path)
    parser.add_argument("--start", type=int, default=1)
    parser.add_argument("--end", type=int, default=17)
    args = parser.parse_args()
    total = 0
    for n in range(args.start, args.end + 1):
        count = verify_order(args.data, n)
        total += count
        print(f"n={n} verified={count} PASS", flush=True)
    print(f"total={total} PASS")


if __name__ == "__main__":
    main()
