import networkx as nx
from collections import Counter

def is_partial_2_tree(G):
    H={v:set(G.neighbors(v)) for v in G}
    active=set(H)
    while active:
        v=next((x for x in active if len(H[x]&active)<=2),None)
        if v is None:
            return False
        ns=list(H[v]&active)
        if len(ns)==2:
            x,y=ns
            H[x].add(y); H[y].add(x)
        active.remove(v)
    return True

cnt=Counter()
reps={}
for G in nx.graph_atlas_g():
    n=len(G)
    if n==0 or n>7: continue
    if nx.is_connected(G) and max(dict(G.degree()).values(),default=0)<=3 and is_partial_2_tree(G):
        cnt[n]+=1
print('atlas_counts',dict(sorted(cnt.items())))
expected={1:1,2:1,3:2,4:5,5:9,6:23,7:50}
assert dict(cnt)==expected,(cnt,expected)
print('PASS')
