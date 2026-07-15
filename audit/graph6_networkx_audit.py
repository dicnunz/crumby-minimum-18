import sys
import networkx as nx

DATA = sys.argv[1] if len(sys.argv) > 1 else "../results/certificates"

def decode_key(n, text):
    words = [int(text[i:i+16], 16) for i in (0, 16, 32)]
    edges = set()
    bit = 0
    for i in range(n):
        for j in range(i + 1, n):
            if (words[bit >> 6] >> (bit & 63)) & 1:
                edges.add((i, j))
            bit += 1
    return edges

for n, stride in [(1, 1), (2, 1), (3, 1), (4, 1), (5, 1), (6, 1),
                  (7, 1), (10, 37), (17, 293)]:
    tested = total = 0
    with open(f"{DATA}/keys_n{n}.txt") as key_file, \
         open(f"{DATA}/graphs_n{n}.g6", "rb") as graph_file:
        for key_line, graph_line in zip(key_file, graph_file):
            if total % stride == 0:
                graph = nx.from_graph6_bytes(graph_line.strip())
                edges = {tuple(sorted(edge)) for edge in graph.edges()}
                assert len(graph) == n
                assert edges == decode_key(n, key_line.strip())
                tested += 1
            total += 1
    print(f"n={n} lines={total} tested={tested} PASS")
