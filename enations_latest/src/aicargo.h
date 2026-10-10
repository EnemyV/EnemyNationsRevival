#ifndef AICARGO_H
#define AICARGO_H

//  Carrier capacity arithmetic, by cargo WEIGHT as the engine charges it
//  (LoadCarrier in netapi.cpp: infantry 1 slot, a vehicle MAX_CARGO slots).
//  Header-only and dependency-free so tests/ai compiles the shipped text.

namespace enaicargo {

//  base.h's MAX_CARGO, mirrored so tests need no game headers; caitmgr.cpp
//  static_asserts the two match.
const int kSlotsPerVehicle = 5;

//  Slots one cargo object costs: infantry 1, anything else a whole vehicle.
inline int CargoCost( bool bIsPeople )
{
    return ( bIsPeople ? 1 : kSlotsPerVehicle );
}

//  Never negative: an over-full carrier reports 0, not a budget that passes.
inline int FreeSlots( int iOccupied, int iCapacity )
{
    if ( iOccupied < 0 )
        iOccupied = 0;
    if ( iCapacity <= iOccupied )
        return ( 0 );
    return ( iCapacity - iOccupied );
}

//  The engine's load gate on numbers: iOccupied = GetCargoSize(), iCapacity =
//  GetEffPeopleCarry(). iReserved = slots promised to cargo NOT yet aboard.
inline bool HasRoomFor( int iOccupied, int iCapacity, int iCost, int iReserved = 0 )
{
    if ( iCost <= 0 )
        return ( false );
    if ( iReserved < 0 )
        iReserved = 0;
    return ( iCost <= FreeSlots( iOccupied + iReserved, iCapacity ) );
}

//  Whole vehicles one carrier can hold; 0 = it cannot take a vehicle at all.
inline int VehiclesPerCarrier( int iCapacity )
{
    if ( iCapacity < kSlotsPerVehicle )
        return ( 0 );
    return ( iCapacity / kSlotsPerVehicle );
}

//  Minimum carriers for nVehicles + nPeople. A bin-pack, not aggregate capacity
//  (two craft with 4 free slots cannot take one tank); with two item sizes,
//  vehicles first then infantry in the slack is optimal. -1 = cannot be lifted.
inline int CarriersNeeded( int nVehicles, int nPeople, int iCapacity )
{
    if ( nVehicles < 0 )
        nVehicles = 0;
    if ( nPeople < 0 )
        nPeople = 0;
    if ( !nVehicles && !nPeople )
        return ( 0 );
    if ( iCapacity <= 0 )
        return ( -1 );

    int iCarriers = 0;
    int iSlack    = 0;

    if ( nVehicles )
    {
        int iPerCraft = VehiclesPerCarrier( iCapacity );
        if ( iPerCraft <= 0 )
            return ( -1 );  // no craft of this type can lift a single vehicle
        iCarriers = ( nVehicles + iPerCraft - 1 ) / iPerCraft;
        // slots left over in the craft the vehicles already occupy
        iSlack = ( iCarriers * iCapacity ) - ( nVehicles * kSlotsPerVehicle );
    }

    int iLeftOver = nPeople - iSlack;
    if ( iLeftOver > 0 )
        iCarriers += ( iLeftOver + iCapacity - 1 ) / iCapacity;

    return ( iCarriers );
}

}  // namespace enaicargo

#endif  // AICARGO_H
