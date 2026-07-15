import networkx as nx
from collections import defaultdict

def partial2(G):
    H={v:set(G[v]) for v in G}
    active=set(H)
    while active:
        v=next((x for x in active if len(H[x]&active)<=2),None)
        if v is None: return False
        ns=list(H[v]&active)
        if len(ns)==2:
            x,y=ns; H[x].add(y); H[y].add(x)
        active.remove(v)
    return True

def inv(G):
    deg=tuple(sorted(dict(G.degree()).values()))
    wl=nx.weisfeiler_lehman_graph_hash(G, iterations=len(G), digest_size=16)
    return (len(G.edges()),deg,wl)

def dedup(cands):
    buckets=defaultdict(list)
    for G in cands:
        b=buckets[inv(G)]
        if not any(nx.is_isomorphic(G,H) for H in b):
            b.append(G)
    return [G for b in buckets.values() for G in b]

levels={1:[nx.empty_graph(1)]}
expected={1:1,2:1,3:2,4:5,5:9,6:23,7:50,8:138,9:354,10:1028}
for n in range(1,10):
    c=[]
    for G in levels[n]:
        deg=dict(G.degree())
        for u in G:
            if deg[u]<3:
                H=G.copy(); H.add_node(n); H.add_edge(u,n); c.append(H)
        for u in G:
            if deg[u]>=3: continue
            for v in range(u+1,n):
                if deg[v]>=3: continue
                H=G.copy(); H.add_node(n); H.add_edges_from([(u,n),(v,n)])
                if partial2(H): c.append(H)
    levels[n+1]=dedup(c)
    print(n+1,len(levels[n+1]))
    assert len(levels[n+1])==expected[n+1]
print('PASS through n=10')
