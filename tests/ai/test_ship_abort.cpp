#include "seek_scan_fixture.h"
using CObject=CAIUnit;
constexpr int CAI_LOC_X=0, CAI_PREV_X=2, CAI_DEST_X=4;
constexpr int CAI_IN_USE=1, CAI_IS_CARGO=8;
struct CMaterialTypes {enum{num_types=10};};
template<class... T> void HpLog(const char*,T...){}
struct CHPRouter {
    CAIUnitList *m_plUnits,*m_plTrucksAvailable,*m_plBldgsNeed,*m_plShipsAvailable;
    int m_iPlayer=1;
    bool IsValidUnit(CAIUnit* p){return p && p->owner==m_iPlayer;}
    BOOL IsShip(DWORD);
    BOOL IsVehicleCargo(DWORD);
    void UnAssignShip(CAIUnit*);
    void ReleasePickupShips(DWORD);
    void UnassignTrucks(DWORD);
    void UnassignTrucks(CAIUnit*);
};
#include "ship_abort_actual.inc"

struct World {
    CAIUnit truck,ship,otherShip,building;
    CVehicle realTruck,realShip,realOtherShip;
    CAIUnitList units,trucks,buildings,ships;
    CHPRouter router;
    World():router{&units,&trucks,&buildings,&ships} {
        lockCalls=lockDepth=arrivals=0;theVehicleMap.items.clear();
        truck.id=1;truck.target=10;truck.status=CAI_IN_USE;
        ship.id=2;ship.status=CAI_IN_USE;ship.paramsDW[CAI_LOC_X]=1;ship.paramsDW[CAI_PREV_X]=20;
        otherShip.id=3;otherShip.status=CAI_IN_USE;otherShip.paramsDW[CAI_LOC_X]=99;otherShip.paramsDW[CAI_PREV_X]=20;
        building.id=10;building.type=CUnit::building;building.paramsDW[0]=1;
        units.items={&truck,&ship,&otherShip,&building};
        realTruck.id=1;realShip.id=2;realOtherShip.id=3;
        realShip.data.type=realOtherShip.data.type=CTransportData::light_cargo;
        theVehicleMap.items={{1,&realTruck},{2,&realShip},{3,&realOtherShip}};
    }
    bool released()const{return ship.status==0 && ship.paramsDW[CAI_LOC_X]==0 && ships.items.size()==1 && ships.items[0]==&ship;}
};
int main(){
    int passed=0,failed=0;
    auto check=[&](bool good,const char* text){std::cout<<(good?"PASS ":"FAIL ")<<text<<'\n';good?++passed:++failed;};
    {World w;w.router.UnassignTrucks(DWORD{1});check(w.released(),"truck deletion releases pickup ship");check(w.otherShip.status==CAI_IN_USE,"unrelated pairing retained");w.router.UnassignTrucks(DWORD{1});check(w.ships.items.size()==1,"release is idempotent");}
    {World w;w.units.items.erase(w.units.items.begin());w.router.UnassignTrucks(DWORD{1});check(w.released(),"missing truck record still releases pickup ship");}
    {World w;w.router.UnassignTrucks(&w.building);check(w.released(),"building material assignment abort releases pickup ship");check(w.trucks.items.size()==1,"truck availability preserved");}
    {World w;w.building.paramsDW[0]=0;w.router.UnassignTrucks(&w.building);check(w.released(),"building fallback sweep abort releases pickup ship");}
    {World w;w.ship.paramsDW[CAI_DEST_X]=30;w.router.UnassignTrucks(&w.building);check(w.ship.status==CAI_IN_USE && w.ship.paramsDW[CAI_DEST_X]==30 && w.ships.items.empty(),"unload leg is not reassigned");}
    {World w;w.realTruck.carrier=&w.realShip;w.router.UnassignTrucks(DWORD{1});check(w.ship.status==CAI_IN_USE && w.ships.items.empty(),"physical cargo is not reassigned");}
    {World w;w.realShip.cargoCount=1;theVehicleMap.items.erase(1);w.router.UnassignTrucks(DWORD{1});check(w.ship.status==CAI_IN_USE && w.ships.items.empty(),"loaded ship retained even when truck is missing");}
    {World w;w.realShip.flags=CUnit::dying;w.router.UnassignTrucks(DWORD{1});check(w.ships.items.empty(),"dying ship not resurrected into available list");}
    {World w;w.ship.owner=2;w.router.UnassignTrucks(DWORD{1});check(w.ship.status==CAI_IN_USE && w.ships.items.empty(),"other owner untouched");}
    {World w;w.ship.status=0;w.router.UnassignTrucks(DWORD{1});check(w.ships.items.empty(),"already idle ship not treated as aborted assignment");}
    check(lockDepth==0,"lock balanced");
    std::cout<<"RESULT passed="<<passed<<" failed="<<failed<<'\n';return failed?1:0;
}
