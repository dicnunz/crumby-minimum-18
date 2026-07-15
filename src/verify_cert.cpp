#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
using std::array; using std::string; using std::uint32_t; using std::uint64_t;
constexpr int NMAX=20;
struct G{int n=0;array<uint32_t,NMAX>a{};};
struct Key{array<uint64_t,3>w{};bool operator==(Key const&o)const{return w==o.w;}bool operator<(Key const&o)const{return w<o.w;}};
int pc(uint32_t x){return std::popcount(x);}
Key parse_key(string const&s){if(s.size()!=48)throw std::runtime_error("bad key length");Key k;for(int i=0;i<3;++i)k.w[i]=std::stoull(s.substr(16*i,16),nullptr,16);return k;}
Key label_key(G const&g){Key k;int b=0;for(int i=0;i<g.n;++i)for(int j=i+1;j<g.n;++j,++b)if((g.a[i]>>j)&1U)k.w[b>>6]|=1ULL<<(b&63);return k;}
G parse_g6(string const&s){if(s.empty())throw std::runtime_error("empty graph6");int n=(unsigned char)s[0]-63;if(n<0||n>62)throw std::runtime_error("unsupported graph6 n");int m=n*(n-1)/2, need=1+(m+5)/6;if((int)s.size()!=need)throw std::runtime_error("bad graph6 length");G g;g.n=n;int bit=0;for(int j=1;j<n;++j)for(int i=0;i<j;++i,++bit){int z=((unsigned char)s[1+bit/6]-63);int val=(z>>(5-bit%6))&1;if(val){g.a[i]|=1U<<j;g.a[j]|=1U<<i;}}return g;}
bool connected(G const&g){uint32_t seen=1,front=1;while(front){uint32_t nx=0,t=front;while(t){int v=std::countr_zero(t);t&=t-1;nx|=g.a[v];}nx&=~seen;seen|=nx;front=nx;}return pc(seen)==g.n;}
bool partial2(G const&g){bool e[NMAX][NMAX]{};bool active[NMAX]{};for(int i=0;i<g.n;++i){active[i]=true;for(int j=0;j<g.n;++j)e[i][j]=(g.a[i]>>j)&1U;}int left=g.n;while(left){int v=-1;for(int x=g.n-1;x>=0;--x)if(active[x]){int d=0;for(int y=0;y<g.n;++y)d+=active[y]&&e[x][y];if(d<=2){v=x;break;}}if(v<0)return false;int ns[2],q=0;for(int y=0;y<g.n;++y)if(active[y]&&e[v][y])ns[q++]=y;if(q==2)e[ns[0]][ns[1]]=e[ns[1]][ns[0]]=true;active[v]=false;--left;}return true;}
bool crumby(G const&g,uint32_t red){uint32_t all=(1U<<g.n)-1U,blue=all^red;for(int v=0;v<g.n;++v)if((blue>>v)&1U){if(pc(g.a[v]&blue)>1)return false;}for(int v=0;v<g.n;++v)if((red>>v)&1U){if(!(g.a[v]&red))return false;}for(int a=0;a<g.n;++a)if((red>>a)&1U){uint32_t tb=g.a[a]&red;while(tb){int b=std::countr_zero(tb);tb&=tb-1;uint32_t tc=g.a[b]&red&~(1U<<a);while(tc){int c=std::countr_zero(tc);tc&=tc-1;uint32_t td=g.a[c]&red&~((1U<<a)|(1U<<b));if(td)return false;}}}return true;}
int main(int argc,char**argv){if(argc!=5){std::cerr<<"usage: verify_cert n keys graphs.g6 witnesses\n";return 2;}int n=std::atoi(argv[1]);std::ifstream kf(argv[2]),gf(argv[3]),wf(argv[4]);string ks,gs,ws;Key prev;bool have=false;uint64_t count=0;while(true){bool a=(bool)std::getline(kf,ks),b=(bool)std::getline(gf,gs),c=(bool)std::getline(wf,ws);if(!(a||b||c))break;if(!(a&&b&&c)){std::cerr<<"line-count mismatch at "<<count<<"\n";return 3;}Key k=parse_key(ks);if(have&&!(prev<k)){std::cerr<<"keys not strictly sorted at "<<count<<"\n";return 4;}G g=parse_g6(gs);if(g.n!=n||!(label_key(g)==k)){std::cerr<<"key/graph mismatch at "<<count<<"\n";return 5;}for(int v=0;v<n;++v)if(pc(g.a[v])>3){std::cerr<<"degree failure at "<<count<<"\n";return 6;}if(!connected(g)){std::cerr<<"connectivity failure at "<<count<<"\n";return 7;}if(!partial2(g)){std::cerr<<"K4-minor/treewidth failure at "<<count<<"\n";return 8;}uint32_t red=(uint32_t)std::stoul(ws,nullptr,16);if(red>>n){std::cerr<<"mask range failure at "<<count<<"\n";return 9;}if(!crumby(g,red)){std::cerr<<"color failure at "<<count<<"\n";return 10;}prev=k;have=true;++count;}std::cout<<"n="<<n<<" verified="<<count<<" PASS\n";}
