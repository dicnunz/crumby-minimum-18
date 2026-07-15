#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;struct K{array<uint64_t,3>w{};};struct G{int n;array<uint32_t,20>a{};};
K parse(string s){K k;for(int i=0;i<3;i++)k.w[i]=stoull(s.substr(16*i,16),0,16);return k;}G dec(int n,K k){G g;g.n=n;int b=0;for(int i=0;i<n;i++)for(int j=i+1;j<n;j++,b++)if((k.w[b>>6]>>(b&63))&1){g.a[i]|=1u<<j;g.a[j]|=1u<<i;}return g;}
bool conn_without(G const&g,int z){if(g.n==1)return true;uint32_t active=((1u<<g.n)-1)&~(1u<<z);int s=countr_zero(active);uint32_t seen=1u<<s,fr=seen;while(fr){uint32_t nx=0,t=fr;while(t){int v=countr_zero(t);t&=t-1;nx|=g.a[v];}nx&=active&~seen;seen|=nx;fr=nx;}return seen==active;}
int main(int ac,char**av){if(ac!=3)return 2;int n=atoi(av[1]);ifstream f(av[2]);string s;uint64_t c=0;while(getline(f,s)){G g=dec(n,parse(s));bool ok=n==1;for(int v=0;v<n&&!ok;v++)if(popcount(g.a[v])<=2&&conn_without(g,v))ok=true;if(!ok){cerr<<"fail "<<c<<"\n";return 3;}c++;}cout<<"n="<<n<<" checked="<<c<<" removable-low-degree-noncut PASS\n";}
