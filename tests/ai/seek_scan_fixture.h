// Minimal world fixture for compiling the actual targeting functions in isolation.
// The test runner extracts their bodies from the selected game source tree.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using DWORD = uint32_t;
using WORD = uint16_t;
using BYTE = uint8_t;
using BOOL = int;
using POSITION = size_t;
constexpr BOOL TRUE = 1, FALSE = 0;
#define ASSERT_VALID(x) ((void)0)
#define ASSERT(x) ((void)0)
#define THREADS_ENABLED 0
#define EN_SEEK_SINGLEPASS 1
constexpr int CAI_SOFTATTACK=6, CAI_HARDATTACK=7, CAI_NAVALATTACK=8;
constexpr int CAI_TARGETTYPE=9, CAI_SPOTTING=2, CAI_UNASSIGNED=10;
constexpr int CAI_TASKSWITCH=1, CAI_IN_COMBAT=2;
constexpr int BEST_TARGET=310, NEAREST_TARGET=311, THREAT_TARGET=312;
constexpr int IDT_SEEKATSEA=1, IDT_SEEKINWAR=2, IDT_SEEKINRANGE=3, IDT_PREPAREWAR=4;
constexpr int NUM_COMBINED_UNITS=32;
inline int caTargetAttack[4096]{};
inline int cs=0, lockCalls=0, lockDepth=0, arrivals=0;
inline void EnterCriticalSection(int*) {
    if (++lockCalls > 100000) throw std::runtime_error("nonreturning seek: test lock-call limit");
    ++lockDepth;
}
inline void LeaveCriticalSection(int*) { --lockDepth; }

struct CHexCoord {
    int x=0,y=0;
    CHexCoord()=default;
    CHexCoord(int a,int b):x(a),y(b){}
    int X() const{return x;} int Y()const{return y;}
    void X(int a){x=a;} void Y(int a){y=a;}
    static int Wrap(int a){return (a%128+128)%128;}
};
struct CSubHex {int x=0,y=0; explicit CSubHex(CHexCoord h):x(h.X()*2),y(h.Y()*2){} };
struct CPlayer {int id=0; int GetPlyrNum()const{return id;}};
struct CUnit {
    enum UNIT_TYPE {building,vehicle};
    enum {dying=1,abandoned=2};
    DWORD id=0,flags=0; CPlayer owner; CHexCoord hex;
    DWORD GetID()const{return id;} DWORD GetFlags()const{return flags;}
    bool IsFlag(DWORD f)const{return (flags&f)!=0;}
    CPlayer* GetOwner(){return &owner;}
};
struct CUnitData {enum {soft,hard,naval,num_attacks};};
struct CStructureData {
    enum {city=0,rocket=1,barracks_2=2,num_types=32};
    int type=rocket;
    int GetType()const{return type;} int GetTargetType()const{return CUnitData::hard;}
};
struct CTransportData {
    enum {light_cargo=13};
    int type=0;
    int GetType()const{return type;} int GetTargetType()const{return CUnitData::soft;}
    int _GetRange()const{return 2;}
};
struct CVehicle:CUnit {
    CTransportData data; CVehicle* carrier=nullptr; int cargoCount=0;
    CHexCoord GetHexHead()const{return hex;} CHexCoord GetHexDest()const{return hex;}
    int GetRange()const{return 2;} int GetSpottingRange()const{return 4;}
    int GetAttack(int)const{return 1;}
    CVehicle* GetTransport()const{return carrier;}
    int GetCargoCount()const{return cargoCount;}
    const CTransportData* GetData()const{return &data;}
};
struct CBuilding:CUnit {
    CStructureData data;
    CHexCoord GetExitHex()const{return hex;}
    int GetRange()const{return 2;} int GetSpottingRange()const{return 4;}
    int GetAttack(int)const{return 1;}
    const CStructureData* GetData()const{return &data;}
};
struct VehicleMap {
    std::map<DWORD,CVehicle*> items;
    CVehicle* GetVehicle(DWORD id)const {
        auto p=items.find(id);
        return p==items.end() || p->second->IsFlag(CUnit::dying) ? nullptr : p->second;
    }
};
struct BuildingMap {
    std::map<DWORD,CBuilding*> items;
    CBuilding* GetBldg(DWORD id)const {
        auto p=items.find(id);
        return p==items.end() || p->second->IsFlag(CUnit::dying) ? nullptr : p->second;
    }
};
struct VehicleHex {
    std::map<std::pair<int,int>,CVehicle*> items;
    CVehicle* GetVehicle(int x,int y)const {
        auto p=items.find({x,y}); return p==items.end()?nullptr:p->second;
    }
};
struct BuildingHex {
    std::map<std::pair<int,int>,CBuilding*> items;
    CBuilding* GetBuilding(CHexCoord h)const {
        auto p=items.find({h.X(),h.Y()});return p==items.end()?nullptr:p->second;
    }
};
inline VehicleMap theVehicleMap;
inline BuildingMap theBuildingMap;
inline VehicleHex theVehicleHex;
inline BuildingHex theBuildingHex;
struct CHex {enum {ul=1,ur=2,ll=4,lr=8,bldg=16,unit=31}; BYTE units=0; BYTE GetUnits()const{return units;}};
struct Map {
    std::map<std::pair<int,int>,CHex> items;
    CHex* GetHex(CHexCoord h){auto p=items.find({h.X(),h.Y()});return p==items.end()?nullptr:&p->second;}
};
inline Map theMap;
struct CAIUnit {
    DWORD id=0,target=0;int owner=1,type=CUnit::vehicle,typeUnit=0,task=IDT_SEEKINWAR,goal=1,status=0;
    WORD params[16]{}; DWORD paramsDW[16]{};
    DWORD GetID()const{return id;} int GetOwner()const{return owner;}
    int GetType()const{return type;} int GetTypeUnit()const{return typeUnit;}
    DWORD GetDataDW()const{return target;} void SetDataDW(DWORD d){target=d;}
    WORD GetTask()const{return static_cast<WORD>(task);} void SetTask(int d){task=d;}
    int GetGoal()const{return goal;} void SetGoal(int d){goal=d;}
    WORD GetStatus()const{return static_cast<WORD>(status);} void SetStatus(WORD d){status=d;}
    WORD GetParam(int i)const{return params[i];} void SetParam(int i,WORD v){params[i]=v;}
    DWORD GetParamDW(int i)const{return paramsDW[i];} void SetParamDW(int i,DWORD v){paramsDW[i]=v;}
    void ClearParam(){std::memset(params,0,sizeof(params));std::memset(paramsDW,0,sizeof(paramsDW));}
    void AttackUnit(DWORD){} void AttackedBy(DWORD){} void SetDestination(CHexCoord){++arrivals;}
};
struct CAIUnitList {
    std::vector<CAIUnit*> items;
    size_t GetCount()const{return items.size();}
    CAIUnit* GetUnitNY(DWORD id){for(auto p:items)if(p->id==id)return p;return nullptr;}
    POSITION AddTail(CAIUnit* p){items.push_back(p);return items.size();}
    CAIUnit* GetOpForUnit(DWORD id) {
        // Fixtures pre-populate known enemies: this is the production cache-hit path.
        for(auto p:items)if(p->id==id)return p;
        return nullptr;
    }
    POSITION GetHeadPosition()const{return items.empty()?0:1;}
    CAIUnit* GetNext(POSITION& p){auto q=items.at(p-1);p=p<items.size()?p+1:0;return q;}
};
struct CAIOpFor {bool ai=false,war=true;bool IsAI()const{return ai;}bool AtWar()const{return war;}};
struct OpFors {CAIOpFor enemy; CAIOpFor* GetOpFor(int id){return id==2?&enemy:nullptr;}};
struct MapUtil {
    bool reachable=true;
    int GetPathRating(CHexCoord,CHexCoord,int)const{return reachable?1:0;}
    int AssessTarget(CVehicle*,int)const{return 1;}
    int AssessTarget(CBuilding*,int)const{return 1;}
    void FindStagingHex(CHexCoord h,int,int,int,CHexCoord& out,BOOL){out=h;}
};
struct AIMap {MapUtil util;MapUtil* m_pMapUtil=&util;};
struct GameData {
    int m_iHexPerBlk=16,m_iSmart=1;
    int GetRangeDistance(CHexCoord a,CHexCoord b)const{return std::max(std::abs(a.X()-b.X()),std::abs(a.Y()-b.Y()));}
};
inline GameData gameData;
inline GameData* pGameData=&gameData;
struct Transports {CTransportData data;const CTransportData* GetData(int)const{return &data;}};
inline Transports theTransports;
struct CAIGoalMgr {
    int m_iPlayer=1;CAIUnitList* m_plUnits=nullptr;OpFors* m_plOpFors=nullptr;AIMap* m_pMap=nullptr;
    int AssessThreat(CVehicle*,int)const{return 1;}int AssessThreat(CBuilding*,int)const{return 1;}
    DWORD GetOpForUnitScan(const int*,const int*,int,CAIUnit*,int*);
};
struct CAITask {int id=IDT_SEEKINWAR;int GetID()const{return id;}};
struct CAITaskMgr {
    CAIGoalMgr* m_pGoalMgr=nullptr;
    bool InRange(CAIUnit*,CHexCoord)const{return true;}
    void UnloadCargo(CAIUnit*){} void MoveToRange(CAIUnit*,CHexCoord){++arrivals;}
    void AssignPatrol(CAIUnit* u){u->SetTask(0);}
    void ClearTaskUnit(CAIUnit* u){u->ClearParam();u->SetTask(0);u->SetGoal(0);u->SetDataDW(0);u->SetStatus(0);}
    void SeekOpfor(CAIUnit*,CAITask*);
};
