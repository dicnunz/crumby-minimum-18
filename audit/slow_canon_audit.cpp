#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using std::array;
using std::cerr;
using std::cout;
using std::endl;
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
    bool operator==(Key const& o) const noexcept { return w == o.w; }
    bool operator<(Key const& o) const noexcept { return w < o.w; }
};
struct KeyHash {
    size_t operator()(Key const& k) const noexcept {
        uint64_t x = k.w[0] + 0x9e3779b97f4a7c15ULL;
        x ^= k.w[1] + 0x9e3779b97f4a7c15ULL + (x<<6) + (x>>2);
        x ^= k.w[2] + 0x9e3779b97f4a7c15ULL + (x<<6) + (x>>2);
        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27; x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return (size_t)x;
    }
};

static inline int pc(uint32_t x) { return __builtin_popcount(x); }

struct Canonicalizer {
    Graph const* g = nullptr;
    Key best{};
    bool have = false;
    uint64_t leaves = 0;

    using Part = vector<vector<int>>;

    Part initial_partition() const {
        std::map<int, vector<int>> bydeg;
        for (int v=0; v<g->n; ++v) bydeg[pc(g->a[v])].push_back(v);
        Part p;
        for (auto &kv: bydeg) p.push_back(kv.second);
        return p;
    }

    Part refine(Part p) const {
        while (true) {
            Part q;
            q.reserve(p.size()*2);
            bool changed = false;
            for (auto const& cell: p) {
                std::map<vector<unsigned char>, vector<int>> groups;
                for (int v: cell) {
                    vector<unsigned char> sig;
                    sig.reserve(p.size());
                    for (auto const& target: p) {
                        int c = 0;
                        for (int u: target) c += ((g->a[v] >> u) & 1U);
                        sig.push_back((unsigned char)c);
                    }
                    groups[sig].push_back(v);
                }
                if (groups.size() != 1) changed = true;
                for (auto &kv: groups) q.push_back(std::move(kv.second));
            }
            p.swap(q);
            if (!changed) return p;
        }
    }

    Key encode(Part const& p) const {
        assert((int)p.size()==g->n);
        vector<int> ord(g->n);
        for (int i=0;i<g->n;++i) { assert(p[i].size()==1); ord[i]=p[i][0]; }
        Key k;
        int bit=0;
        // Canonical-key order: lexicographic list (0,1),(0,2),...,(0,n-1),(1,2),...
        for (int i=0;i<g->n;++i) for (int j=i+1;j<g->n;++j,++bit) {
            if ((g->a[ord[i]] >> ord[j]) & 1U) k.w[bit>>6] |= 1ULL << (bit&63);
        }
        return k;
    }

    void search(Part p) {
        p = refine(std::move(p));
        int pick = -1;
        for (int i=0;i<(int)p.size();++i) if (p[i].size()>1) { pick=i; break; }
        if (pick<0) {
            ++leaves;
            Key k=encode(p);
            if (!have || k<best) { best=k; have=true; }
            return;
        }
        vector<int> cell=p[pick];
        for (int v: cell) {
            Part q;
            q.reserve(p.size()+1);
            for (int i=0;i<pick;++i) q.push_back(p[i]);
            q.push_back(vector<int>{v});
            vector<int> rest;
            rest.reserve(cell.size()-1);
            for (int u: cell) if (u!=v) rest.push_back(u);
            if (!rest.empty()) q.push_back(std::move(rest));
            for (int i=pick+1;i<(int)p.size();++i) q.push_back(p[i]);
            search(std::move(q));
        }
    }

    Key run(Graph const& gr) {
        g=&gr; have=false; best=Key{}; leaves=0;
        search(initial_partition());
        assert(have);
        return best;
    }
};

Graph decode_key(int n, Key const& k) {
    Graph g; g.n=n;
    int bit=0;
    for (int i=0;i<n;++i) for (int j=i+1;j<n;++j,++bit) {
        if ((k.w[bit>>6] >> (bit&63)) & 1ULL) {
            g.a[i] |= 1U<<j;
            g.a[j] |= 1U<<i;
        }
    }
    return g;
}

// Exact elimination-order characterization of treewidth <= 2.
// We branch over every currently degree-at-most-2 vertex, fill its neighborhood,
// delete it, and accept iff some complete elimination order exists.
struct TW2Checker {
    // Exact reduction test for treewidth <= 2.  If v has current degree at most 2,
    // eliminating v (and, for degree 2, adding the edge between its neighbors)
    // preserves the property tw <= 2 in both directions.  In the forward direction
    // the filled graph is obtained by deleting v (degree 0/1) or contracting one
    // edge incident with v (degree 2), hence is a minor.  In the reverse direction,
    // append v first in an elimination order.  Therefore any low-degree choice is safe.
    bool run(Graph const& g) const {
        auto a=g.a;
        uint32_t active=(1U<<g.n)-1U;
        while(active){
            int chosen=-1;
            uint32_t t=active;
            while(t){int v=__builtin_ctz(t);t&=t-1;
                if(pc(a[v]&active)<=2){chosen=v;break;}
            }
            if(chosen<0) return false;
            uint32_t nb=a[chosen]&active;
            if(pc(nb)==2){
                int x=__builtin_ctz(nb); nb&=nb-1;
                int y=__builtin_ctz(nb);
                a[x]|=1U<<y; a[y]|=1U<<x;
            }
            active&=~(1U<<chosen);
        }
        return true;
    }
};

bool connected(Graph const& g) {
    if (g.n==0) return false;
    uint32_t seen=1, frontier=1;
    while (frontier) {
        uint32_t next=0, t=frontier;
        while(t){ int v=__builtin_ctz(t); t&=t-1; next |= g.a[v]; }
        next &= ~seen;
        seen |= next; frontier=next;
    }
    return pc(seen)==g.n;
}

bool subcubic(Graph const& g) {
    for(int v=0;v<g.n;++v) if(pc(g.a[v])>3) return false;
    return true;
}

vector<uint32_t> simple_p4_masks(Graph const& g) {
    std::set<uint32_t> ss;
    for (int a=0;a<g.n;++a) {
      uint32_t nb1=g.a[a];
      while(nb1){ int b=__builtin_ctz(nb1); nb1&=nb1-1; if(b==a) continue;
        uint32_t nb2=g.a[b]&~(1U<<a);
        while(nb2){ int c=__builtin_ctz(nb2); nb2&=nb2-1; if(c==b) continue;
          uint32_t nb3=g.a[c]&~((1U<<a)|(1U<<b));
          while(nb3){ int d=__builtin_ctz(nb3); nb3&=nb3-1;
            if(d==c) continue;
            ss.insert((1U<<a)|(1U<<b)|(1U<<c)|(1U<<d));
          }
        }
      }
    }
    return vector<uint32_t>(ss.begin(),ss.end());
}

bool is_crumby_mask(Graph const& g, uint32_t red) {
    uint32_t all=(1U<<g.n)-1U, blue=all^red;
    uint32_t t=blue;
    while(t){ int v=__builtin_ctz(t); t&=t-1; if(pc(g.a[v]&blue)>1) return false; }
    t=red;
    while(t){ int v=__builtin_ctz(t); t&=t-1; if((g.a[v]&red)==0) return false; }
    auto p4=simple_p4_masks(g);
    for(uint32_t m:p4) if((red&m)==m) return false;
    return true;
}

// A tiny complete DPLL solver for the CNF encoding. It only needs to find a witness.
struct Clause { uint32_t pos=0, neg=0; }; // pos vars appear x, neg vars appear !x

struct CrumbySAT {
    Graph const* g=nullptr;
    vector<Clause> c;
    uint32_t full=0;

    void build(Graph const& gr) {
        g=&gr; full=(1U<<gr.n)-1U; c.clear();
        // If v,u,w are all blue and u,w are distinct neighbors of v, blue degree at v exceeds 1.
        for(int v=0;v<gr.n;++v){
            vector<int> nb;
            uint32_t t=gr.a[v]; while(t){int u=__builtin_ctz(t);t&=t-1;nb.push_back(u);}
            for(int i=0;i<(int)nb.size();++i) for(int j=i+1;j<(int)nb.size();++j)
                c.push_back(Clause{(1U<<v)|(1U<<nb[i])|(1U<<nb[j]),0});
        }
        // Every red v has a red neighbor: !x_v OR (OR neighbors x_u).
        for(int v=0;v<gr.n;++v) c.push_back(Clause{gr.a[v],1U<<v});
        // Every simple 4-vertex path contains a blue vertex.
        for(uint32_t m:simple_p4_masks(gr)) c.push_back(Clause{0,m});
    }

    // returns -1 conflict, 0 unresolved, 1 all clauses satisfied. Performs units in-place.
    int propagate(uint32_t &assigned, uint32_t &truth) const {
        while(true){
            bool any=false;
            bool allsat=true;
            for(auto const& cl:c){
                uint32_t falsemask=assigned&~truth;
                if((cl.pos&truth) || (cl.neg&falsemask)) continue;
                allsat=false;
                uint32_t upos=cl.pos&~assigned;
                uint32_t uneg=cl.neg&~assigned;
                uint32_t u=upos|uneg;
                if(!u) return -1;
                if((u&(u-1))==0){
                    int v=__builtin_ctz(u);
                    assigned|=1U<<v;
                    if(upos&(1U<<v)) truth|=1U<<v;
                    else truth&=~(1U<<v);
                    any=true;
                    break;
                }
            }
            if(any) continue;
            return allsat?1:0;
        }
    }

    bool dfs(uint32_t assigned, uint32_t truth, uint32_t &answer) const {
        int st=propagate(assigned,truth);
        if(st<0) return false;
        if(st>0){ answer=truth; return true; }
        // Choose an unassigned variable from a shortest unresolved clause, with occurrence tie-break.
        uint32_t falsemask=assigned&~truth;
        int bestlen=100; uint32_t candidates=0;
        for(auto const& cl:c){
            if((cl.pos&truth)||(cl.neg&falsemask)) continue;
            uint32_t u=(cl.pos|cl.neg)&~assigned;
            int len=pc(u);
            if(len<bestlen){bestlen=len;candidates=u;}
        }
        int bestv=-1,bestscore=-1;
        uint32_t ut=full&~assigned;
        while(ut){int v=__builtin_ctz(ut);ut&=ut-1; if(!(candidates&(1U<<v)) && candidates) continue;
            int sc=0; for(auto const& cl:c) if((cl.pos|cl.neg)&(1U<<v)) ++sc;
            if(sc>bestscore){bestscore=sc;bestv=v;}
        }
        if(bestv<0) bestv=__builtin_ctz(full&~assigned);
        uint32_t bit=1U<<bestv;
        // Try red first, then blue.
        if(dfs(assigned|bit,truth|bit,answer)) return true;
        if(dfs(assigned|bit,truth&~bit,answer)) return true;
        return false;
    }

    bool solve(Graph const& gr, uint32_t &answer) {
        build(gr);
        bool ok=dfs(0,0,answer);
        if(ok) answer &= full;
        return ok;
    }
};

string graph6(Graph const& g){
    assert(g.n<=62);
    string bits;
    bits.reserve(g.n*(g.n-1)/2);
    // graph6 order: (0,1),(0,2),(1,2),(0,3),(1,3),(2,3),...
    for(int j=1;j<g.n;++j) for(int i=0;i<j;++i)
        bits.push_back(((g.a[i]>>j)&1U)?'1':'0');
    while(bits.size()%6) bits.push_back('0');
    string s; s.push_back(char(g.n+63));
    for(size_t i=0;i<bits.size();i+=6){int x=0;for(int k=0;k<6;++k)x=(x<<1)|(bits[i+k]-'0');s.push_back(char(x+63));}
    return s;
}

string hexkey(Key const& k){
    std::ostringstream os;
    os<<std::hex<<std::setfill('0')<<std::setw(16)<<k.w[0]<<std::setw(16)<<k.w[1]<<std::setw(16)<<k.w[2];
    return os.str();
}

Graph permute_graph(Graph const& g, uint64_t &seed){
    int p[MAXN]; for(int i=0;i<g.n;++i)p[i]=i;
    for(int i=g.n-1;i>0;--i){seed^=seed<<7;seed^=seed>>9;seed^=seed<<8;int j=(int)(seed%(uint64_t)(i+1));std::swap(p[i],p[j]);}
    Graph h;h.n=g.n;for(int i=0;i<g.n;++i)for(int j=i+1;j<g.n;++j)if((g.a[i]>>j)&1U){h.a[p[i]]|=1U<<p[j];h.a[p[j]]|=1U<<p[i];}return h;
}
int main(int argc,char**argv){if(argc<4){std::cerr<<"usage keys n stride\n";return 2;}std::ifstream f(argv[1]);int n=std::atoi(argv[2]),stride=std::atoi(argv[3]);string line;uint64_t idx=0,tested=0,seed=0x123456789abcdef0ULL;Canonicalizer can;while(std::getline(f,line)){if(idx%(uint64_t)stride==0){Key k;for(int z=0;z<3;++z)k.w[z]=std::stoull(line.substr(16*z,16),nullptr,16);Graph g=decode_key(n,k);Key a=can.run(g);if(!(a==k)){std::cerr<<"canonical fixed-point mismatch at "<<idx<<" expected="<<hexkey(k)<<" got="<<hexkey(a)<<"\n";return 3;}Graph h=permute_graph(g,seed);Key b=can.run(h);if(!(b==k)){std::cerr<<"permutation mismatch at "<<idx<<" expected="<<hexkey(k)<<" got="<<hexkey(b)<<"\n";return 4;}++tested;}++idx;}std::cout<<"n="<<n<<" lines="<<idx<<" tested="<<tested<<" PASS\n";}
