#include "common.hpp"
#include <omp.h>

using namespace crumby;

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: color_keys n input_keys output_witnesses threads exhaustive_on_negative\n";
        return 2;
    }
    const int n = std::atoi(argv[1]);
    const string input_name = argv[2];
    const string output_name = argv[3];
    const int threads = std::atoi(argv[4]);
    const bool exhaustive_on_negative = std::atoi(argv[5]) != 0;

    vector<Key> keys;
    {
        std::ifstream input(input_name);
        string line;
        while (std::getline(input, line)) keys.push_back(parse_hex_key(line));
    }

    vector<uint32_t> witnesses(keys.size(), 0);
    vector<unsigned char> status(keys.size(), 0);
    uint64_t colorable = 0;
    uint64_t uncolorable = 0;
    omp_set_num_threads(threads);

    #pragma omp parallel
    {
        CrumbySAT solver;
        TW2Checker tw2;
        #pragma omp for schedule(dynamic, 256) reduction(+:colorable,uncolorable)
        for (long long i = 0; i < static_cast<long long>(keys.size()); ++i) {
            Graph g = decode_key(n, keys[static_cast<size_t>(i)]);
            if (!connected(g) || !subcubic(g) || !tw2.run(g)) {
                status[static_cast<size_t>(i)] = 10;
                continue;
            }

            uint32_t witness = 0;
            if (solver.solve(g, witness)) {
                if (!is_crumby_mask(g, witness)) {
                    status[static_cast<size_t>(i)] = 11;
                    continue;
                }
                witnesses[static_cast<size_t>(i)] = witness;
                status[static_cast<size_t>(i)] = 1;
                ++colorable;
                continue;
            }

            bool found = false;
            if (exhaustive_on_negative) {
                for (uint32_t red = 0; red < (1U << n); ++red) {
                    if (is_crumby_mask(g, red)) {
                        found = true;
                        witness = red;
                        break;
                    }
                }
            }
            if (found) {
                status[static_cast<size_t>(i)] = 12;
                witnesses[static_cast<size_t>(i)] = witness;
            } else {
                status[static_cast<size_t>(i)] = 2;
                ++uncolorable;
            }
        }
    }

    for (size_t i = 0; i < keys.size(); ++i) {
        if (status[i] >= 10) {
            std::cerr << "internal failure status=" << static_cast<int>(status[i])
                      << " index=" << i << " key=" << hex_key(keys[i])
                      << " witness=" << witnesses[i] << '\n';
            return 3;
        }
    }

    std::ofstream output(output_name);
    for (uint32_t witness : witnesses) {
        output << std::hex << std::setfill('0') << std::setw(8) << witness << '\n';
    }

    std::cout << n << '\t' << keys.size() << '\t' << colorable << '\t' << uncolorable << '\n';
    return 0;
}
