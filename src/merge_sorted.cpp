#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

struct Item {
    std::string line;
    int stream = 0;
};
struct Later {
    bool operator()(Item const& a, Item const& b) const { return a.line > b.line; }
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: merge_sorted output input1 input2 ...\n";
        return 2;
    }
    std::ofstream output(argv[1]);
    std::vector<std::ifstream> inputs;
    for (int i = 2; i < argc; ++i) inputs.emplace_back(argv[i]);

    std::priority_queue<Item, std::vector<Item>, Later> heap;
    for (int i = 0; i < static_cast<int>(inputs.size()); ++i) {
        std::string line;
        if (std::getline(inputs[i], line)) heap.push(Item{line, i});
    }

    std::string previous;
    bool have_previous = false;
    unsigned long long input_lines = 0;
    unsigned long long unique_lines = 0;
    while (!heap.empty()) {
        Item item = heap.top();
        heap.pop();
        ++input_lines;
        if (!have_previous || item.line != previous) {
            output << item.line << '\n';
            previous = item.line;
            have_previous = true;
            ++unique_lines;
        }
        std::string next;
        if (std::getline(inputs[item.stream], next)) heap.push(Item{next, item.stream});
    }

    std::cout << input_lines << '\t' << unique_lines << '\t'
              << (input_lines - unique_lines) << '\n';
    return 0;
}
