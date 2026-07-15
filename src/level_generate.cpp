#include "common.hpp"
#include <omp.h>

using namespace crumby;

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: level_generate n input_keys output_keys threads buckets\n";
        return 2;
    }
    const int n = std::atoi(argv[1]);
    const string input_name = argv[2];
    const string output_name = argv[3];
    const int threads = std::atoi(argv[4]);
    const int bucket_count = std::atoi(argv[5]);
    if (n < 1 || n >= MAXN || threads < 1 || bucket_count < 1 ||
        (bucket_count & (bucket_count - 1)) != 0) {
        std::cerr << "invalid argument\n";
        return 2;
    }

    vector<Key> parents;
    {
        std::ifstream input(input_name);
        string line;
        while (std::getline(input, line)) parents.push_back(parse_hex_key(line));
    }

    omp_set_num_threads(threads);
    vector<vector<vector<Key>>> buckets(
        static_cast<size_t>(threads),
        vector<vector<Key>>(static_cast<size_t>(bucket_count))
    );
    vector<uint64_t> leaf_counts(threads, 0);
    vector<uint64_t> pair_counts(threads, 0);
    vector<uint64_t> reject_counts(threads, 0);
    vector<uint64_t> canonical_leaves(threads, 0);
    KeyHash hash;

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        Canonicalizer canonicalizer;
        TW2Checker tw2;
        auto& local_buckets = buckets[tid];

        #pragma omp for schedule(dynamic, 64)
        for (long long index = 0; index < static_cast<long long>(parents.size()); ++index) {
            Graph g = decode_key(n, parents[static_cast<size_t>(index)]);
            if (!connected(g) || !subcubic(g) || !tw2.run(g)) {
                std::cerr << "invalid parent at index " << index << "\n";
                std::abort();
            }
            int degree[MAXN]{};
            for (int v = 0; v < n; ++v) degree[v] = popcount(g.a[v]);

            for (int u = 0; u < n; ++u) {
                if (degree[u] >= 3) continue;
                ++leaf_counts[tid];
                Graph h = g;
                h.n = n + 1;
                h.a[n] = 1U << u;
                h.a[u] |= 1U << n;
                Key key = canonicalizer.run(h);
                canonical_leaves[tid] += canonicalizer.leaves;
                local_buckets[hash(key) & static_cast<size_t>(bucket_count - 1)].push_back(key);
            }

            for (int u = 0; u < n; ++u) {
                if (degree[u] >= 3) continue;
                for (int v = u + 1; v < n; ++v) {
                    if (degree[v] >= 3) continue;
                    ++pair_counts[tid];
                    Graph h = g;
                    h.n = n + 1;
                    h.a[n] = (1U << u) | (1U << v);
                    h.a[u] |= 1U << n;
                    h.a[v] |= 1U << n;
                    if (!tw2.run(h)) {
                        ++reject_counts[tid];
                        continue;
                    }
                    Key key = canonicalizer.run(h);
                    canonical_leaves[tid] += canonicalizer.leaves;
                    local_buckets[hash(key) & static_cast<size_t>(bucket_count - 1)].push_back(key);
                }
            }
        }
    }

    uint64_t raw_leaf = 0;
    uint64_t raw_pair = 0;
    uint64_t rejected = 0;
    uint64_t leaves = 0;
    for (int t = 0; t < threads; ++t) {
        raw_leaf += leaf_counts[t];
        raw_pair += pair_counts[t];
        rejected += reject_counts[t];
        leaves += canonical_leaves[t];
    }

    vector<vector<Key>> unique_buckets(static_cast<size_t>(bucket_count));
    #pragma omp parallel for schedule(dynamic, 1)
    for (int b = 0; b < bucket_count; ++b) {
        size_t size = 0;
        for (int t = 0; t < threads; ++t) size += buckets[t][b].size();
        auto& merged = unique_buckets[static_cast<size_t>(b)];
        merged.reserve(size);
        for (int t = 0; t < threads; ++t) {
            auto& source = buckets[t][b];
            merged.insert(merged.end(), source.begin(), source.end());
            vector<Key>().swap(source);
        }
        std::sort(merged.begin(), merged.end());
        merged.erase(std::unique(merged.begin(), merged.end()), merged.end());
    }

    size_t local_unique = 0;
    for (auto const& bucket : unique_buckets) local_unique += bucket.size();
    vector<Key> output_keys;
    output_keys.reserve(local_unique);
    for (auto& bucket : unique_buckets) {
        output_keys.insert(output_keys.end(), bucket.begin(), bucket.end());
        vector<Key>().swap(bucket);
    }
    std::sort(output_keys.begin(), output_keys.end());
    output_keys.erase(std::unique(output_keys.begin(), output_keys.end()), output_keys.end());

    std::ofstream output(output_name);
    for (Key const& key : output_keys) output << hex_key(key) << '\n';

    const uint64_t accepted = raw_leaf + raw_pair - rejected;
    std::cout
        << parents.size() << '\t'
        << raw_leaf << '\t'
        << raw_pair << '\t'
        << rejected << '\t'
        << accepted << '\t'
        << leaves << '\t'
        << output_keys.size() << '\n';
    return 0;
}
