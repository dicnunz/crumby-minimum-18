#include "common.hpp"
using namespace crumby;

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: keys_to_g6 n input_keys output_g6\n";
        return 2;
    }
    const int n = std::atoi(argv[1]);
    std::ifstream input(argv[2]);
    std::ofstream output(argv[3], std::ios::binary);
    string line;
    uint64_t count = 0;
    while (std::getline(input, line)) {
        output << graph6(decode_key(n, parse_hex_key(line))) << '\n';
        ++count;
    }
    std::cout << n << '\t' << count << '\n';
    return 0;
}
