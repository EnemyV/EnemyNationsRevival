// Production parking/request/lane methods and flee guard; replace scene services.
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using BOOL = int;
using DWORD = unsigned long;
constexpr BOOL TRUE = 1, FALSE = 0;
constexpr int PARK_SEARCH_SUBS = 16, CORRIDOR_MIN_HEXES = 3, CORRIDOR_MIN_VEHS = 3;
constexpr int HOLD_FRAMES = 240, MAX_NUM_RETRIES = 25;
int TrafficOpts() { return 63; }
void WaitLog(const char*, ...) {}
struct CHexCoord;
struct CSubHex {
    int x = 100, y = 100;
    CSubHex() = default;
    CSubHex(int a,int b): x(a),y(b) {}
    void Wrap() { x=(x+2048)%2048; y=(y+2048)%2048; }
    static int Diff(int d) { return (d+3072)%2048-1024; }
    CHexCoord ToCoord() const;
};
bool operator==(CSubHex a,CSubHex b) { return a.x==b.x&&a.y==b.y; }
bool operator!=(CSubHex a,CSubHex b) { return !(a==b); }
struct CHexCoord {
    int x=50,y=50;
    CHexCoord()=default;
    CHexCoord(int a,int b):x(a),y(b) {}
    CHexCoord(CSubHex s):x(s.x/2),y(s.y/2) {}
    int X() const { return x; } int Y() const { return y; }
};
bool operator==(CHexCoord a,CHexCoord b) { return a.x==b.x&&a.y==b.y; }
CHexCoord CSubHex::ToCoord() const { return CHexCoord(*this); }
struct CHex {
    enum { bldg=1,bridge=2,road=9,city=0,lake=3,plain=7 };
    int units=0,type=plain;
    int GetUnits() const { return units; } int GetType() const { return type; }
};
struct Map {
    CHex hex;
    CHex* _GetHex(CHexCoord) { return &hex; }
    CHex* _GetHex(CSubHex) { return &hex; }
    int Get_eX() const { return 1024; } int Get_eY() const { return 1024; }
    int GetTerrainCost(CHexCoord,CHexCoord,int,int) { return hex.type==CHex::lake&&!(hex.units&CHex::bridge)?0:2; }
} theMap;
struct Owner { bool local=true; BOOL IsLocal() const { return local; } };
struct Data {
    enum { FL1hex=1 };
    BOOL IsBoat() const { return FALSE; } BOOL IsTransport() const { return TRUE; }
    BOOL IsCrane() const { return FALSE; } int GetType() const { return 2; }
    int GetWheelType() const { return 1; } int GetVehFlags() const { return 0; }
    BOOL CanEnterHex(CHexCoord,CHexCoord,BOOL,BOOL) const { return TRUE; }
    BOOL CanTravelHex(CHex* h) const { return h->type!=CHex::lake||(h->units&CHex::bridge); }
};
using CTransportData=Data;
struct Paths {
    int calls=0,failFirst=0;
    CHexCoord* GetPath(void*,CHexCoord&,CHexCoord& to,int& length,int,BOOL,BOOL) {
        ++calls; length=0;
        if(calls<=failFirst) return nullptr;
        length=1; return new CHexCoord[1]{to};
    }
} thePathMgr;
constexpr int EVENT_CONST_UNDER_ATK=1,EVENT_WARN=2;
struct Game {
    DWORD now=10000; int alerts=0;
    DWORD GettimeGetTime() const { return now; }
    int GetFramesElapsed() const { return 1; }
    void Event(int,int,void*) { ++alerts; }
} theGame;
struct CVehicle {
    enum { stop=0,moving=1,blocked=4,none=0,route=1,build=2,stopped=4,dying=8,told_ai_stop=16 };
    CSubHex m_ptHead{100,100},m_ptTail{99,100},m_ptNext{101,100},m_ptDest{120,100};
    bool m_bReversing=false,m_bForwardEscape=false,m_bConfined=false,m_bResume=false;
    int m_iHoldFrames=0,m_iParkSkip=0,m_iNumRetries=13,m_cMode=stop,m_iEvent=none;
    int flags=0,id=1,orders=0;
    int m_unitFlags=0,m_bFlags=told_ai_stop,resumes=0;
    BOOL m_cOwn=TRUE,clearHead=TRUE,clearTail=TRUE,hasJob=TRUE;
    DWORD m_dwAskedToMove=0;
    void* m_pBldg=nullptr;
    Owner* owner=nullptr; Data data;
    BOOL manual=false,eligible=true,laneBlocked=true;
    Owner* GetOwner() const { return owner; } Data* GetData() { return &data; }
    int GetID() const { return id; } CSubHex GetPtHead() const { return m_ptHead; }
    CHexCoord GetHexHead() const { return CHexCoord(m_ptHead); }
    BOOL IsFlag(int f) const { return flags&f; } BOOL IsHpControl() const { return manual; }
    BOOL JamEligible() const { return eligible&&!flags&&owner&&owner->local; }
    int CorridorAhead(int& count) { count=0;return 0; }
    BOOL OnPavement(CSubHex) { return theMap.hex.type==CHex::road||theMap.hex.type==CHex::city||(theMap.hex.units&CHex::bridge); }
    BOOL IsShoreline(CHexCoord) { return FALSE; }
    BOOL BlockedLaneStep(CSubHex& p) { p=m_ptNext;return laneBlocked; }
    CSubHex Rotate(int d) { return {m_ptHead.x+1,m_ptHead.y+d}; }
    BOOL CanEnter(CSubHex) { return FALSE; }
    void DetourTo(CSubHex,BOOL) { ++orders; }
    BOOL FindOffRoadSpot(CSubHex&,CVehicle*);
    BOOL AskToMove(CVehicle*);
    BOOL MustKeepLane(CSubHex&);
    BOOL ClearOfRoad(CSubHex p) { return p==m_ptHead?clearHead:clearTail; }
    BOOL ResumeJob() { ++resumes;return hasJob; }
    void TickHold();
};
struct VehicleMap {
    CVehicle* blocker=nullptr;
    CVehicle* _GetVehicle(CSubHex) { return blocker; }
} theVehicleHex;
#include "parking_actual.inc"
int checks=0,failures=0;
void check(bool ok,const char* msg) { ++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",msg);} }
int main() {
    Owner own,other,remote;remote.local=false;
    CVehicle a,b;a.owner=&own;b.owner=&other;b.m_ptHead=a.m_ptNext;b.eligible=false;
    theVehicleHex.blocker=&b;CSubHex step;
    check(!a.MustKeepLane(step),"local fixed blocker permits existing passing exception");
    b.owner=&remote;check(a.MustKeepLane(step),"remote endpoint stop must not permit passing");
    b.flags=CVehicle::stopped;check(a.MustKeepLane(step),"remote flags do not override local-authority requirement");
    b.owner=&own;b.flags=0;b.eligible=true;check(a.MustKeepLane(step),"cooperating own queue keeps lane");
    theVehicleHex.blocker=nullptr;
    thePathMgr.calls=0;thePathMgr.failFirst=8;CSubHex target;
    bool found=a.FindOffRoadSpot(target,nullptr);
    check(!found&&thePathMgr.calls==8,"one parking invocation has at most eight path searches");
    check(a.m_iParkSkip==8,"failed batch remembers next candidate");
    if(!found) {
        check(a.FindOffRoadSpot(target,nullptr),"later invocation reaches ninth candidate");
        check(thePathMgr.calls==9&&a.m_iParkSkip==0,"success clears continuation");
    }
    a.m_iParkSkip=0;thePathMgr.calls=0;thePathMgr.failFirst=1000000;
    check(!a.AskToMove(&a),"self request ignored");b.owner=&own;
    check(!a.AskToMove(&b),"failed request does not invent an unchecked destination");
    int previous=thePathMgr.calls;
    check(!a.AskToMove(&b)&&thePathMgr.calls==previous,"failed request is throttled too");
    theGame.now+=5000;a.AskToMove(&b);
    check(thePathMgr.calls==previous+8,"retry resumes bounded work after cooldown");
    for(int type:{CHex::road,CHex::city,CHex::plain}) {
        theMap.hex.type=type;thePathMgr.calls=0;thePathMgr.failFirst=0;
        check(TestFleeTarget(&a,{110,100}),"reachable paved/ordinary ground permits emergency flee");
    }
    theMap.hex.type=CHex::lake;theMap.hex.units=0;
    check(!TestFleeTarget(&a,{110,100}),"water rejected");
    theMap.hex.type=CHex::plain;theMap.hex.units=CHex::bldg;
    check(!TestFleeTarget(&a,{110,100}),"building rejected");
    theMap.hex.units=0;thePathMgr.failFirst=1000000;
    check(!TestFleeTarget(&a,{110,100}),"unreachable ground rejected");
    check(theGame.alerts==3,"rejecting a flee destination still warns about the attack");
    CVehicle h;h.owner=&own;h.m_iHoldFrames=240;h.clearTail=FALSE;
    h.TickHold();
    check(h.m_iHoldFrames==0&&h.resumes==1,"failed parking with tail on road cancels hold and resumes job");
    h.m_iHoldFrames=240;h.clearTail=TRUE;h.clearHead=FALSE;h.resumes=0;h.hasJob=FALSE;
    h.TickHold();
    check(h.m_iHoldFrames==0&&!(h.m_bFlags&CVehicle::told_ai_stop),"failed parking without job re-enables ordinary notification");
    h.clearHead=TRUE;h.m_iHoldFrames=240;h.resumes=0;h.TickHold();
    check(h.m_iHoldFrames==239&&h.resumes==0,"safe stopped parking counts down without premature resume");
    h.clearTail=FALSE;h.m_cMode=CVehicle::moving;h.TickHold();
    check(h.m_iHoldFrames==239&&h.resumes==0,"in-progress clearing retains the complete pending hold");
    h.m_cMode=CVehicle::stop;h.m_unitFlags=CVehicle::stopped;h.TickHold();
    check(h.m_iHoldFrames==239&&h.resumes==0,"explicit Stop prevents held-job revival");
    h.m_unitFlags=0;h.m_cOwn=FALSE;h.TickHold();
    check(h.m_iHoldFrames==239&&h.resumes==0,"unowned carried/interior pose does not resume on a hold tick");
    std::printf("%d checks, %d failures\n",checks,failures);return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
