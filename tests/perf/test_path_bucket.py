#!/usr/bin/env python3
"""Emit standalone C++ equivalence test using current production method bodies.

python test_path_bucket.py > path_bucket_test.cpp
Compile as C++17 and run. No game headers, SDL, save or window required.
"""
from pathlib import Path
import re


def method(source, name, owner="CPathMgr"):
    match = re.search(r"(?:CCell\s*\*|void)\s*" + owner + "::" + name + r"\s*\([^)]*\)\s*\{", source)
    if not match:
        raise ValueError("method not found: " + name)
    depth = 1
    end = match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def main():
    src = Path(__file__).resolve().parents[2] / "enations_latest/src"
    source = (src / "cpathmgr.cpp").read_text(encoding="utf-8")
    bodies = "\n".join(method(source, name) for name in ("xGetLowestCost", "GetLowestCost", "NewBoth", "ClearArray"))
    header = (src / "cpathmgr.h").read_text(encoding="utf-8")
    assert re.search(r"const int MAX_BOTH_INDEX\s*=\s*4096\s*;", header), "shared constant changed"
    begin = source.index("m_iBestCost  = ( pAdjCell->m_iCost + pAdjCell->m_iDist );")
    end_text = "while ( iEnd-- ) NewBoth( pCell++ );"
    end = source.index(end_text, begin) + len(end_text)
    phase = source[begin:end].replace("( pAdjCell->m_iCost + pAdjCell->m_iDist )", "best")
    bodies += "\nvoid CPathMgr::RebuildPhase(int best) {\n" + phase + "\n}\n"
    map_source = (src / "cpathmap.cpp").read_text(encoding="utf-8")
    map_bodies = "\n".join(method(map_source, name, "CPathMap")
                           for name in ("xGetLowestCost", "GetLowestCost", "NewBoth", "ClearArray"))
    assignment = "m_iLowestBoth = (int)( ppCellOn - m_acBoth );"
    assert bodies.count(assignment) == 1, "expected production lower-bound update"
    bodies = bodies.replace(assignment, "if (optimized) " + assignment)
    loop = "while ( ( *ppCellOn == NULL ) && ( iNum-- > 0 ) ) ppCellOn++;"
    assert bodies.count(loop) == 1, "expected production bucket scan"
    bodies = bodies.replace(loop, "while ( ( *ppCellOn == NULL ) && ( iNum-- > 0 ) ) { ++scans; ++ppCellOn; }")
    print(r'''
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#ifdef NDEBUG
#error Build this equivalence test with assertions enabled
#endif
#ifndef __min
#define __min(a,b) std::min(a,b)
#endif
using std::min;
constexpr int MAX_BOTH_INDEX = 4096;
struct CCell {
    int m_iDist=0, m_iBoth=0, m_iBothIn=0, m_iX=0, m_iY=0, m_iCost=0;
    CCell *m_pcNextBoth=nullptr, *m_pcPrevBoth=nullptr;
    CCell *m_pCellFrom=nullptr, *m_pCellNext=nullptr;
};
struct MapStub { void RemoveAll() {} };
struct CPathMgr {
    CCell cells[128]{};
    CCell *m_paCells=cells;
    CCell *m_acBoth[MAX_BOTH_INDEX+1]{};
    int m_iNextSlot=128, m_iNumOfCells=128, m_iLowestBoth=0, m_iBestCost=0xFFFE;
    bool optimized;
    uint64_t scans=0;
    MapStub m_mapCell;
    explicit CPathMgr(bool opt):optimized(opt){}
    CCell* xGetLowestCost(int&);
    CCell* GetLowestCost(int&);
    void NewBoth(CCell*);
    void ClearArray();
    void RebuildPhase(int);
    int index(CCell *c) { return c ? int(c-cells) : -1; }
    void set(int i,int cost,int dist) {
        cells[i].m_iBoth=cost; cells[i].m_iDist=dist; NewBoth(&cells[i]);
        m_iNextSlot=std::max(m_iNextSlot,i+1);
    }
};
struct CPathMap {
    CCell cells[128]{};
    CCell *m_paCells=cells;
    CCell *m_acBoth[MAX_BOTH_INDEX+1]{};
    int m_iNextSlot=128, m_iCellAlloc=128, m_iLowestBoth=0, generation=0;
    CCell* xGetLowestCost();
    CCell* GetLowestCost();
    void NewBoth(CCell*);
    void ClearArray();
    void NewSearchGen() { ++generation; }
    int index(CCell *c) { return c ? int(c-cells) : -1; }
};
''')
    print(bodies)
    print(map_bodies)
    print(r'''
static void compare(CPathMgr& old, CPathMgr& changed) {
    int a=-1,b=-1;
    int ia=old.index(old.GetLowestCost(a));
    int ib=changed.index(changed.GetLowestCost(b));
    assert(ia==ib && a==b);
}
static void checkMap(CPathMap& map, const uint64_t* stamps) {
    int expected=-1, best=0xFFFF;
    uint64_t newest=0;
    // CPathMap intentionally returns bucket HEAD on a tie (latest insertion),
    // unlike CPathMgr's arena-first exact tie oracle. Preserve that behavior.
    for(int i=0;i<map.m_iNextSlot;++i) {
        int cost=map.cells[i].m_iBoth;
        if(cost<=0 || cost>=MAX_BOTH_INDEX) continue;
        if(cost<best || (cost==best && stamps[i]>newest)) {
            expected=i; best=cost; newest=stamps[i];
        }
    }
    if(expected<0) {
        best=0xFFFF;
        for(int i=0;i<map.m_iNextSlot;++i)
            if(map.cells[i].m_iBoth && map.cells[i].m_iBoth<best) {
                expected=i;best=map.cells[i].m_iBoth;
            }
    }
    assert(map.index(map.GetLowestCost())==expected);
}
int main() {
    // Exhaust the old minimum; repeatedly select across a 3499-slot empty gap.
    CPathMgr old(false), changed(true);
    for (auto *m : {&old,&changed}) {
        m->set(0,1,1); m->set(1,3500,3500); m->set(2,3500,3500);
        m->set(0,0,0);
    }
    for (int i=0;i<10000;++i) compare(old,changed);
    assert(old.scans==34990000 && changed.scans==3499);
    std::printf("sparse selections=10000 old_empty_scans=%llu new_empty_scans=%llu\n",
                (unsigned long long)old.scans,(unsigned long long)changed.scans);
    // Cheaper reinsertion must lower the cached bound; ties retain arena order.
    for(auto *m : {&old,&changed}) m->set(3,2,2);
    compare(old,changed);
    int tie=-1;
    assert(changed.index(changed.GetLowestCost(tie))==3);
    std::mt19937 rng(1881);
    // Both priority modes; in/out-of-range keys; removal, repricing, ties and
    // empty-list fallback. Cell indices and tie counts must remain identical.
    for (int mode=0;mode<2;++mode) {
        CPathMgr a(false),b(true);
        a.m_iBestCost=b.m_iBestCost=mode?100:0xFFFE;
        for(int n=0;n<200000;++n) {
            int i=int(rng()%128), cost=int(rng()%4500), dist=int(rng()%4500);
            if(n%7==0) cost=0;
            a.set(i,cost,dist); b.set(i,cost,dist);
            compare(a,b);
        }
    }
    // Exercise the production first-destination phase rebuild on the SAME
    // already-advanced cache, then lower best cost without changing key mode.
    // Production ClearArray body resets arena/list state between reused searches.
    CPathMgr a(false),b(true);
    for(int search=0;search<1000;++search) {
        a.ClearArray();b.ClearArray();
        a.m_iNextSlot=b.m_iNextSlot=0;
        a.m_iBestCost=b.m_iBestCost=0xFFFE;
        assert(a.m_iLowestBoth==0 && b.m_iLowestBoth==0);
        compare(a,b);
        for(int i=0;i<128;++i) {
            int cost=int(rng()%4500), dist=int(rng()%4500);
            a.set(i,cost,dist);b.set(i,cost,dist);compare(a,b);
        }
        a.RebuildPhase(200);b.RebuildPhase(200);compare(a,b);
        a.m_iBestCost=b.m_iBestCost=100;compare(a,b);
        for(int n=0;n<128;++n) {
            int i=int(rng()%128),cost=int(rng()%4500),dist=int(rng()%4500);
            a.set(i,cost,dist);b.set(i,cost,dist);compare(a,b);
        }
    }
    // CPathMap uses independent production methods but shares MAX_BOTH_INDEX.
    // Check exact expected head/arena identity, removal, repricing, empty range,
    // sentinel edge, generation-reset/reuse and out-of-bucket fallback.
    CPathMap map;
    uint64_t stamps[128]{};
    uint64_t stamp=0;
    for(int search=0;search<100;++search) {
        map.ClearArray();
        assert(map.m_iLowestBoth==0 && map.generation==search+1);
        for(auto& c:map.cells) c=CCell(); // AddCellToArray initializes recycled slots.
        for(int n=0;n<2000;++n) {
            int i=int(rng()%128),cost=int(rng()%4500);
            if(n%11==0) cost=0;
            if(n%13==0) cost=MAX_BOTH_INDEX;
            map.cells[i].m_iBoth=cost;stamps[i]=++stamp;
            map.NewBoth(&map.cells[i]);checkMap(map,stamps);
        }
    }
    std::puts("PASS: 400000 randomized CPathMgr selections; 1000 reset/phase transitions; 200000 separate CPathMap selections; exact identities/ties");
}
''')


if __name__ == "__main__":
    main()
