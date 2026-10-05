#include "seek_scan_fixture.h"
#include "seek_scan_actual.inc"

struct World {
    CVehicle seeker, enemyVehicle;
    CBuilding enemyBuilding;
    CAIUnit aiSeeker, aiEnemy;
    CAIUnitList units;
    OpFors opfors;
    AIMap map;
    CAIGoalMgr goal;
    CAITaskMgr tasks;
    World(bool building,bool dying,bool known=true) {
        theMap.items.clear();theVehicleMap.items.clear();theBuildingMap.items.clear();
        theVehicleHex.items.clear();theBuildingHex.items.clear();theSnaps.clear();
        lockCalls=lockDepth=arrivals=0;
        seeker.id=1;seeker.owner.id=1;seeker.hex={10,10};
        aiSeeker.id=1;
        aiEnemy.id=2;aiEnemy.owner=2;aiEnemy.type=building?CUnit::building:CUnit::vehicle;
        aiEnemy.typeUnit=building?CStructureData::rocket:0;
        units.items={&aiSeeker};if(known)units.items.push_back(&aiEnemy);
        theVehicleMap.items[1]=&seeker;
        if(building) {
            enemyBuilding.id=2;enemyBuilding.owner.id=2;enemyBuilding.hex={11,10};
            enemyBuilding.flags=dying?CUnit::dying:0;
            theBuildingMap.items[2]=&enemyBuilding;
            theBuildingHex.items[{11,10}]=&enemyBuilding;
            theMap.items[{11,10}].units=CHex::bldg;
        } else {
            enemyVehicle.id=2;enemyVehicle.owner.id=2;enemyVehicle.hex={11,10};
            enemyVehicle.flags=dying?CUnit::dying:0;
            theVehicleMap.items[2]=&enemyVehicle;
            theVehicleHex.items[{22,20}]=&enemyVehicle;
            theMap.items[{11,10}].units=CHex::ul;
        }
        goal.m_plUnits=&units;goal.m_plOpFors=&opfors;goal.m_pMap=&map;
        tasks.m_pGoalMgr=&goal;
    }
};

int main() {
    int failed=0,passed=0;
    auto check=[&](bool good,const char* name){std::cout<<(good?"PASS ":"FAIL ")<<name<<'\n';good?++passed:++failed;};
    for(bool building:{false,true}) {
        const int how[]={NEAREST_TARGET},kind[]={0};
        for(bool dying:{false,true}) {
            World w(building,dying);int selected=-1;
            auto id=w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected);
            check(id==(dying?0U:2U), building?(dying?"dying building excluded":"live building retained"):(dying?"dying vehicle excluded":"live vehicle retained"));
            check(lockDepth==0,"scan balances lock");
        }
        for(int task:{IDT_SEEKINWAR,IDT_SEEKINRANGE,IDT_SEEKATSEA}) {
            World w(building,true);CAITask t;t.id=task;
            bool returned=true;
            try {w.tasks.SeekOpfor(&w.aiSeeker,&t);}catch(const std::runtime_error&){returned=false;}
            check(returned,building?"actual SeekOpfor returns with known dying building":"actual SeekOpfor returns with known dying vehicle");
            if(returned)check(w.aiSeeker.target==0,"no rejected target retained");
        }
        {
            World w(building,false);w.opfors.enemy.ai=true;w.opfors.enemy.war=false;
            int selected=-1;
            check(w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected)==0,"allied AI remains excluded");
        }
        {
            World w(building,false);w.map.util.reachable=false;int selected=-1;
            check(w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected)==0,"unreachable target remains excluded");
        }
        {
            World w(building,true);
            CVehicle survivor;survivor.id=3;survivor.owner.id=2;survivor.hex={12,10};
            CAIUnit aiSurvivor;aiSurvivor.id=3;aiSurvivor.owner=2;
            w.units.items.push_back(&aiSurvivor);
            theVehicleMap.items[3]=&survivor;
            theVehicleHex.items[{24,20}]=&survivor;
            theMap.items[{12,10}].units=CHex::ul;
            int selected=-1;
            check(w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected)==3,"dying nearest does not hide live alternative");
        }
        {
            World w(building,false);w.aiEnemy.owner=1;
            w.enemyBuilding.owner.id=1;w.enemyVehicle.owner.id=1;
            int selected=-1;
            check(w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected)==0,"own unit remains excluded");
        }
    }
    for(auto sub:{std::pair<int,int>{23,20},std::pair<int,int>{22,21}})for(bool threat:{true,false}) {
        // dying enemy in subhex (22,20) of hex 11,10; live enemy 3 in another subhex of the
        // same hex. THREAT has no whole-list fallback; for NEAREST a farther live enemy 4
        // (hex 13,10) would win if 3 stayed hidden.
        World w(false,true);
        CVehicle survivor,farther;
        survivor.id=3;survivor.owner.id=2;survivor.hex={11,10};
        farther.id=4;farther.owner.id=2;farther.hex={13,10};
        CAIUnit aiSurvivor,aiFarther;aiSurvivor.id=3;aiSurvivor.owner=2;aiFarther.id=4;aiFarther.owner=2;
        w.units.items.push_back(&aiSurvivor);w.units.items.push_back(&aiFarther);
        theVehicleMap.items[3]=&survivor;theVehicleMap.items[4]=&farther;
        theVehicleHex.items[sub]=&survivor;theVehicleHex.items[{26,20}]=&farther;
        theMap.items[{11,10}].units=CHex::ul|CHex::ur|CHex::ll;
        theMap.items[{13,10}].units=CHex::ul;
        const int how[]={threat?THREAT_TARGET:NEAREST_TARGET},kind[]={threat?CAI_SOFTATTACK:0};int selected=-1;
        check(w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected)==3,
              threat?"dying subhex does not hide live enemy in same hex (threat)":"dying subhex does not hide live enemy in same hex (nearest)");
    }
    {
        World w(false,false);w.enemyVehicle.carrier=&w.seeker;
        const int how[]={NEAREST_TARGET},kind[]={0};int selected=-1;
        check(w.goal.GetOpForUnitScan(how,kind,1,&w.aiSeeker,&selected)==0,"loaded cargo remains excluded");
    }
    std::cout<<"RESULT passed="<<passed<<" failed="<<failed<<'\n';
    return failed?1:0;
}
