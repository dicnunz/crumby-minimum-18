#ifndef CRUMBY_COMMON_HPP
#define CRUMBY_COMMON_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace crumby {

using std::array;
using std::string;
using std::uint32_t;
using std::uint64_t;
using std::vector;

static constexpr int MAXN = 20;

struct Graph {
    int n = 0;
    array<uint32_t, MAXN> a{};
};

struct Key {
    array<uint64_t, 3> w{};
    bool operator==(Key const& other) const noexcept { return w == other.w; }
    bool operator<(Key const& other) const noexcept { return w < other.w; }
};

struct KeyHash {
    size_t operator()(Key const& k) const noexcept {
        uint64_t x = k.w[0] + 0x9e3779b97f4a7c15ULL;
        x ^= k.w[1] + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2);
        x ^= k.w[2] + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2);
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return static_cast<size_t>(x);
    }
};

inline int popcount(uint32_t x) { return __builtin_popcount(x); }

inline string hex_key(Key const& k) {
    std::ostringstream out;
    out << std::hex << std::setfill('0')
        << std::setw(16) << k.w[0]
        << std::setw(16) << k.w[1]
        << std::setw(16) << k.w[2];
    return out.str();
}

inline Key parse_hex_key(string const& line) {
    if (line.size() != 48) throw std::runtime_error("canonical key must have 48 hex digits");
    Key k;
    for (int i = 0; i < 3; ++i) {
        k.w[i] = std::stoull(line.substr(16 * i, 16), nullptr, 16);
    }
    return k;
}

inline Graph decode_key(int n, Key const& k) {
    Graph g;
    g.n = n;
    int bit = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j, ++bit) {
            if ((k.w[bit >> 6] >> (bit & 63)) & 1ULL) {
                g.a[i] |= 1U << j;
                g.a[j] |= 1U << i;
            }
        }
    }
    return g;
}

class Canonicalizer {
public:
    uint64_t leaves = 0;

    Key run(Graph const& graph) {
        g_ = &graph;
        have_best_ = false;
        best_ = Key{};
        leaves = 0;
        search(initial_partition());
        assert(have_best_);
        return best_;
    }

private:
    struct State {
        array<unsigned char, MAXN> color{};
        int classes = 0;
    };

    Graph const* g_ = nullptr;
    Key best_{};
    bool have_best_ = false;

    State initial_partition() const {
        State s;
        bool present[4] = {false, false, false, false};
        for (int v = 0; v < g_->n; ++v) present[popcount(g_->a[v])] = true;
        int degree_color[4] = {0, 0, 0, 0};
        int k = 0;
        for (int d = 0; d < 4; ++d) if (present[d]) degree_color[d] = k++;
        for (int v = 0; v < g_->n; ++v) {
            s.color[v] = static_cast<unsigned char>(degree_color[popcount(g_->a[v])]);
        }
        s.classes = k;
        return s;
    }

    static bool signature_less(
        int x,
        int y,
        State const& s,
        unsigned char counts[MAXN][MAXN]
    ) {
        if (s.color[x] != s.color[y]) return s.color[x] < s.color[y];
        for (int c = 0; c < s.classes; ++c) {
            if (counts[x][c] != counts[y][c]) return counts[x][c] < counts[y][c];
        }
        // This only fixes traversal order inside one still-unsplit cell.  Equal
        // signatures remain in the same cell and every vertex in that cell is branched on.
        return x < y;
    }

    static bool signature_equal(
        int x,
        int y,
        State const& s,
        unsigned char counts[MAXN][MAXN]
    ) {
        if (s.color[x] != s.color[y]) return false;
        for (int c = 0; c < s.classes; ++c) {
            if (counts[x][c] != counts[y][c]) return false;
        }
        return true;
    }

    State refine(State s) const {
        while (true) {
            unsigned char counts[MAXN][MAXN]{};
            for (int v = 0; v < g_->n; ++v) {
                uint32_t todo = g_->a[v];
                while (todo) {
                    int u = __builtin_ctz(todo);
                    todo &= todo - 1;
                    ++counts[v][s.color[u]];
                }
            }

            int order[MAXN];
            for (int i = 0; i < g_->n; ++i) order[i] = i;
            for (int i = 1; i < g_->n; ++i) {
                int x = order[i];
                int j = i;
                while (j > 0 && signature_less(x, order[j - 1], s, counts)) {
                    order[j] = order[j - 1];
                    --j;
                }
                order[j] = x;
            }

            State next;
            next.classes = 1;
            next.color[order[0]] = 0;
            for (int i = 1; i < g_->n; ++i) {
                if (!signature_equal(order[i - 1], order[i], s, counts)) ++next.classes;
                next.color[order[i]] = static_cast<unsigned char>(next.classes - 1);
            }
            if (next.classes == s.classes) return s;
            s = next;
        }
    }

    Key encode(State const& s) const {
        assert(s.classes == g_->n);
        int order[MAXN];
        for (int v = 0; v < g_->n; ++v) order[s.color[v]] = v;
        Key key;
        int bit = 0;
        for (int i = 0; i < g_->n; ++i) {
            for (int j = i + 1; j < g_->n; ++j, ++bit) {
                if ((g_->a[order[i]] >> order[j]) & 1U) {
                    key.w[bit >> 6] |= 1ULL << (bit & 63);
                }
            }
        }
        return key;
    }

    void search(State state) {
        state = refine(state);
        if (state.classes == g_->n) {
            ++leaves;
            Key key = encode(state);
            if (!have_best_ || key < best_) {
                best_ = key;
                have_best_ = true;
            }
            return;
        }

        int sizes[MAXN]{};
        for (int v = 0; v < g_->n; ++v) ++sizes[state.color[v]];
        int chosen_class = 0;
        while (chosen_class < state.classes && sizes[chosen_class] == 1) ++chosen_class;
        assert(chosen_class < state.classes);

        for (int v = 0; v < g_->n; ++v) {
            if (state.color[v] != chosen_class) continue;
            State next = state;
            for (int u = 0; u < g_->n; ++u) {
                if (u == v) continue;
                if (next.color[u] == chosen_class) {
                    next.color[u] = static_cast<unsigned char>(chosen_class + 1);
                } else if (next.color[u] > chosen_class) {
                    ++next.color[u];
                }
            }
            next.color[v] = static_cast<unsigned char>(chosen_class);
            ++next.classes;
            search(next);
        }
    }
};

class TW2Checker {
public:
    bool run(Graph const& g) const {
        auto adjacency = g.a;
        uint32_t active = (1U << g.n) - 1U;
        while (active) {
            int chosen = -1;
            uint32_t todo = active;
            while (todo) {
                int v = __builtin_ctz(todo);
                todo &= todo - 1;
                if (popcount(adjacency[v] & active) <= 2) {
                    chosen = v;
                    break;
                }
            }
            if (chosen < 0) return false;
            uint32_t neighbors = adjacency[chosen] & active;
            if (popcount(neighbors) == 2) {
                int x = __builtin_ctz(neighbors);
                neighbors &= neighbors - 1;
                int y = __builtin_ctz(neighbors);
                adjacency[x] |= 1U << y;
                adjacency[y] |= 1U << x;
            }
            active &= ~(1U << chosen);
        }
        return true;
    }
};

inline bool connected(Graph const& g) {
    if (g.n == 0) return false;
    uint32_t seen = 1;
    uint32_t frontier = 1;
    while (frontier) {
        uint32_t next = 0;
        uint32_t todo = frontier;
        while (todo) {
            int v = __builtin_ctz(todo);
            todo &= todo - 1;
            next |= g.a[v];
        }
        next &= ~seen;
        seen |= next;
        frontier = next;
    }
    return popcount(seen) == g.n;
}

inline bool subcubic(Graph const& g) {
    for (int v = 0; v < g.n; ++v) if (popcount(g.a[v]) > 3) return false;
    return true;
}

inline vector<uint32_t> simple_p4_masks(Graph const& g) {
    std::set<uint32_t> masks;
    for (int a = 0; a < g.n; ++a) {
        uint32_t nb1 = g.a[a];
        while (nb1) {
            int b = __builtin_ctz(nb1);
            nb1 &= nb1 - 1;
            uint32_t nb2 = g.a[b] & ~(1U << a);
            while (nb2) {
                int c = __builtin_ctz(nb2);
                nb2 &= nb2 - 1;
                uint32_t nb3 = g.a[c] & ~((1U << a) | (1U << b));
                while (nb3) {
                    int d = __builtin_ctz(nb3);
                    nb3 &= nb3 - 1;
                    masks.insert((1U << a) | (1U << b) | (1U << c) | (1U << d));
                }
            }
        }
    }
    return vector<uint32_t>(masks.begin(), masks.end());
}

inline bool is_crumby_mask(Graph const& g, uint32_t red) {
    uint32_t all = (1U << g.n) - 1U;
    uint32_t blue = all ^ red;
    uint32_t todo = blue;
    while (todo) {
        int v = __builtin_ctz(todo);
        todo &= todo - 1;
        if (popcount(g.a[v] & blue) > 1) return false;
    }
    todo = red;
    while (todo) {
        int v = __builtin_ctz(todo);
        todo &= todo - 1;
        if ((g.a[v] & red) == 0) return false;
    }
    for (uint32_t path_mask : simple_p4_masks(g)) {
        if ((red & path_mask) == path_mask) return false;
    }
    return true;
}

struct Clause {
    uint32_t positive = 0;
    uint32_t negative = 0;
};

class CrumbySAT {
public:
    bool solve(Graph const& g, uint32_t& answer) {
        build(g);
        bool ok = dfs(0, 0, answer);
        if (ok) answer &= full_;
        return ok;
    }

private:
    vector<Clause> clauses_;
    uint32_t full_ = 0;

    void build(Graph const& g) {
        full_ = (1U << g.n) - 1U;
        clauses_.clear();

        for (int v = 0; v < g.n; ++v) {
            vector<int> neighbors;
            uint32_t todo = g.a[v];
            while (todo) {
                int u = __builtin_ctz(todo);
                todo &= todo - 1;
                neighbors.push_back(u);
            }
            for (int i = 0; i < static_cast<int>(neighbors.size()); ++i) {
                for (int j = i + 1; j < static_cast<int>(neighbors.size()); ++j) {
                    clauses_.push_back(Clause{
                        (1U << v) | (1U << neighbors[i]) | (1U << neighbors[j]),
                        0
                    });
                }
            }
        }

        for (int v = 0; v < g.n; ++v) {
            clauses_.push_back(Clause{g.a[v], 1U << v});
        }

        for (uint32_t path_mask : simple_p4_masks(g)) {
            clauses_.push_back(Clause{0, path_mask});
        }
    }

    int propagate(uint32_t& assigned, uint32_t& truth) const {
        while (true) {
            bool changed = false;
            bool all_satisfied = true;
            for (Clause const& clause : clauses_) {
                uint32_t false_mask = assigned & ~truth;
                if ((clause.positive & truth) || (clause.negative & false_mask)) continue;
                all_satisfied = false;
                uint32_t unassigned_positive = clause.positive & ~assigned;
                uint32_t unassigned_negative = clause.negative & ~assigned;
                uint32_t unassigned = unassigned_positive | unassigned_negative;
                if (!unassigned) return -1;
                if ((unassigned & (unassigned - 1)) == 0) {
                    int v = __builtin_ctz(unassigned);
                    assigned |= 1U << v;
                    if (unassigned_positive & (1U << v)) truth |= 1U << v;
                    else truth &= ~(1U << v);
                    changed = true;
                    break;
                }
            }
            if (changed) continue;
            return all_satisfied ? 1 : 0;
        }
    }

    bool dfs(uint32_t assigned, uint32_t truth, uint32_t& answer) const {
        int status = propagate(assigned, truth);
        if (status < 0) return false;
        if (status > 0) {
            answer = truth;
            return true;
        }

        uint32_t false_mask = assigned & ~truth;
        int shortest = 100;
        uint32_t candidates = 0;
        for (Clause const& clause : clauses_) {
            if ((clause.positive & truth) || (clause.negative & false_mask)) continue;
            uint32_t unassigned = (clause.positive | clause.negative) & ~assigned;
            int length = popcount(unassigned);
            if (length < shortest) {
                shortest = length;
                candidates = unassigned;
            }
        }

        int best_vertex = -1;
        int best_score = -1;
        uint32_t todo = full_ & ~assigned;
        while (todo) {
            int v = __builtin_ctz(todo);
            todo &= todo - 1;
            if (candidates && !(candidates & (1U << v))) continue;
            int score = 0;
            for (Clause const& clause : clauses_) {
                if ((clause.positive | clause.negative) & (1U << v)) ++score;
            }
            if (score > best_score) {
                best_score = score;
                best_vertex = v;
            }
        }
        if (best_vertex < 0) best_vertex = __builtin_ctz(full_ & ~assigned);

        uint32_t bit = 1U << best_vertex;
        if (dfs(assigned | bit, truth | bit, answer)) return true;
        if (dfs(assigned | bit, truth & ~bit, answer)) return true;
        return false;
    }
};

inline string graph6(Graph const& g) {
    assert(g.n <= 62);
    string bits;
    bits.reserve(g.n * (g.n - 1) / 2);
    for (int j = 1; j < g.n; ++j) {
        for (int i = 0; i < j; ++i) {
            bits.push_back(((g.a[i] >> j) & 1U) ? '1' : '0');
        }
    }
    while (bits.size() % 6) bits.push_back('0');
    string out;
    out.push_back(static_cast<char>(g.n + 63));
    for (size_t i = 0; i < bits.size(); i += 6) {
        int value = 0;
        for (int k = 0; k < 6; ++k) value = (value << 1) | (bits[i + k] - '0');
        out.push_back(static_cast<char>(value + 63));
    }
    return out;
}

} // namespace crumby

#endif
