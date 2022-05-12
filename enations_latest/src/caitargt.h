////////////////////////////////////////////////////////////////////////////
//
//  CAITargt.h : Include file for targeting influences
//               
//  Last update:    07/26/97
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

// 
// represents an influence by type of attacker against all types
// of targets by returning a factor that is multiplied by the
// CMapUtil::AssessTarget() return values, such that if a type
// of attacker is not to attack a target type, the factor returned
// will be 0 and a normal attack is 1, and more desirable targets
// are indicated as factor increases
//
// rows are type of target, 
// columns are type of attacker factor for that target
//
// buildings are zero based
// vehicles are CStructureData::num_types based
//
// (CStructureData::num_types + CTransportData::num_types) = 65

#define NUM_COMBINED_UNITS 65

//
// to find a factor, multiply NUM_COMBINED_UNITS by type of target
// and add type of attacker to get offset, then get factor at offset
//

static BYTE caTargetAttack[] = {
// CStructureData::city as a target
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,
//
// CStructureData::rocket as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_1_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_1_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_1_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_2_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_2_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_2_3 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_2_4 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_3_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::apartment_3_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::office_2_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::office_2_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::office_2_3 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::office_2_4 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::office_3_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::office_3_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::barracks_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::barracks_3 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::command_center as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::embassy as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::farm as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
2,2,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::fort_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::fort_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::fort_3 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::heavy as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::lumber as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::oil_well as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::refinery as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::coal as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::iron as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::copper as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::power_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::power_2 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::power_3 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::research as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,2,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::repair as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::seaport as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::shipyard_1 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::shipyard_3 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::smelter as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
2,2,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,2,3,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::warehouse as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
1,1,1,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_0 as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::construction as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,4, // construction,med_truck,heavy_truck,light_scout,
4,4,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,0,0,0, // heavy_art,light_cargo,gun_boat,destroyer,
0,0,0,0,0, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::med_truck as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,4, // construction,med_truck,heavy_truck,light_scout,
4,4,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,0,0,0, // heavy_art,light_cargo,gun_boat,destroyer,
0,0,0,0,0, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::heavy_truck as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,4, // construction,med_truck,heavy_truck,light_scout,
4,4,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,0,0,0, // med_tank,heavy_tank,light_art,med_art,
0,0,0,0, // heavy_art,light_cargo,gun_boat,destroyer,
0,0,0,0,0, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_scout as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,4, // construction,med_truck,heavy_truck,light_scout,
4,4,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
0,0,1,0, // med_tank,heavy_tank,light_art,med_art,
0,0,1,0, // heavy_art,light_cargo,gun_boat,destroyer,
0,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::med_scout as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,4, // construction,med_truck,heavy_truck,light_scout,
4,4,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
0,0,1,0, // med_tank,heavy_tank,light_art,med_art,
0,0,1,0, // heavy_art,light_cargo,gun_boat,destroyer,
0,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::heavy_scout as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,4, // construction,med_truck,heavy_truck,light_scout,
4,4,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::infantry_carrier as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,2, // construction,med_truck,heavy_truck,light_scout,
4,4,4,3, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,2,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,4,4,4,4, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_tank as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,1,1, // med_tank,heavy_tank,light_art,med_art,
1,1,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::med_tank as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
3,3,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,2,2, // heavy_art,light_cargo,gun_boat,destroyer,
2,2,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::heavy_tank as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
3,4,1,2, // med_tank,heavy_tank,light_art,med_art,
3,0,2,3, // heavy_art,light_cargo,gun_boat,destroyer,
4,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_art as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,3,2, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::med_art as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,3,3, // med_tank,heavy_tank,light_art,med_art,
2,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::heavy_art as a target
//
2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,
3,3,3,2,2,3,3,3, // barracks,ccc,forts
3,3,3,2,2,2,2,2, // heavy,light_1,light_2
2,2,2,2,2,2,2,2,
2,2,2,2,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,3,3, // med_tank,heavy_tank,light_art,med_art,
3,0,1,2, // heavy_art,light_cargo,gun_boat,destroyer,
3,2,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::light_cargo as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1, // barracks,ccc,forts
1,1,1,1,1,1,1,1, // heavy,light_1,light_2
1,1,1,1,1,1,2,2, // shipyards
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,1,1, // med_tank,heavy_tank,light_art,med_art,
1,0,3,3, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::gun_boat as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,2,2, // barracks,ccc,forts
1,1,1,1,1,1,1,1, // heavy,light_1,light_2
1,1,1,1,1,1,3,3, // shipyards
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,1,2, // med_tank,heavy_tank,light_art,med_art,
2,0,4,3, // heavy_art,light_cargo,gun_boat,destroyer,
1,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::destroyer as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,2,2, // barracks,ccc,forts
1,1,1,1,1,1,1,1, // heavy,light_1,light_2
1,1,1,1,1,1,3,3, // shipyards
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,1,2, // med_tank,heavy_tank,light_art,med_art,
3,0,4,4, // heavy_art,light_cargo,gun_boat,destroyer,
3,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::cruiser as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,2,2, // barracks,ccc,forts
1,1,1,1,1,1,1,1, // heavy,light_1,light_2
1,1,1,1,1,1,4,4, // shipyards
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,1,3, // med_tank,heavy_tank,light_art,med_art,
4,0,2,3, // heavy_art,light_cargo,gun_boat,destroyer,
4,1,1,1,1, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::landing_craft as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,2,1,1,2,2,2, // barracks,ccc,forts
1,1,1,1,1,2,2,2, // heavy,light_1,light_2
1,1,1,1,1,2,4,4, // shipyards
1,1,1,1,
0,0,0,1, // construction,med_truck,heavy_truck,light_scout,
1,1,1,1, // med_scout,heavy_scout,infantry_carrier,light_tank,
2,2,2,2, // med_tank,heavy_tank,light_art,med_art,
2,0,3,3, // heavy_art,light_cargo,gun_boat,destroyer,
2,3,2,2,2, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::infantry as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
2,2,2,1,1,2,2,2, // barracks,ccc,forts
1,1,1,1,1,2,2,2, // heavy,light_1,light_2
1,1,1,1,1,1,2,2, // shipyards
1,1,1,1,
0,0,0,3, // construction,med_truck,heavy_truck,light_scout,
3,3,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,1,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,2,4,3,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::rangers as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
2,2,2,1,1,2,2,2, // barracks,ccc,forts
1,1,1,1,1,2,2,2, // heavy,light_1,light_2
1,1,1,1,1,1,2,2, // shipyards
1,1,1,1,
0,0,0,3, // construction,med_truck,heavy_truck,light_scout,
3,3,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,2,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,3,3,4,3, // cruiser,landing_craft,infantry,rangers,marines
//
// CStructureData::marines as a target
//
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
2,2,2,1,1,2,2,2, // barracks,ccc,forts
1,1,1,1,1,2,2,2, // heavy,light_1,light_2
1,1,1,1,1,1,2,2, // shipyards
1,1,1,1,
0,0,0,3, // construction,med_truck,heavy_truck,light_scout,
3,3,2,2, // med_scout,heavy_scout,infantry_carrier,light_tank,
1,1,2,1, // med_tank,heavy_tank,light_art,med_art,
1,0,2,1, // heavy_art,light_cargo,gun_boat,destroyer,
1,3,3,3,4 // cruiser,landing_craft,infantry,rangers,marines
};

/*
		enum BLDG_TYPE { city,								// this is returned by GetType ()
						rocket,
						apartment_1_1,			// #=frontier/estab/city, #=choice
						apartment_1_2,
						apartment_2_1,
						apartment_2_2,
						apartment_2_3,
						apartment_2_4,
						apartment_3_1,			// obsolete
						apartment_3_2,			// obsolete
						office_2_1,
						office_2_2,
						office_2_3,
						office_2_4,
						office_3_1,
						office_3_2,					// obsolete
						// above are only placed by computer
						barracks_2,
						barracks_3,					// obsolete
						command_center,
						embassy,
						farm,
						fort_1,
						fort_2,
						fort_3,
						heavy,
						light_1,
						light_2,
						lumber,
						oil_well,
						refinery,
						coal,
						iron,
						copper,
						power_1,
						power_2,
						power_3,
						research,
						repair,
						seaport,
						shipyard_1,
						shipyard_3,
						smelter,
						warehouse,
						light_0,
						num_types, = 44 buildings

		enum TRANS_TYPE {  construction,		// this is returned by GetType ()
						med_truck,					// obsolete
						heavy_truck,
						light_scout,
						med_scout,
						heavy_scout,
						infantry_carrier,
						light_tank,
						med_tank,
						heavy_tank,
						light_art,
						med_art,
						heavy_art,
						light_cargo,
						gun_boat,
						destroyer,
						cruiser,
						landing_craft,
						infantry,
						rangers,
						marines, 						// obsolete
						num_types = 21 vehicles
						};
*/
