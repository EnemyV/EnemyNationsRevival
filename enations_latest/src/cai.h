////////////////////////////////////////////////////////////////////////////
//
//  CAI.h :  General Defines used by
//           Divide and Conquer AI
//               
//  Last update:    07/26/96
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc. - All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////

// this is where all game headers should come in?
#include "racedata.h"
#include "error.h"

// toggles all thread yielding from the programs
#define THREADS_ENABLED	1

// toggles debug output in pathing modules
#define DEBUG_OUTPUT_PATH	0
#define PATH_TIMING			0
#define PATH_TIMING_ROAD	0
#define PATH_TIMING_MAP		0

// ids for menu in data editor
#define IDM_QUIT		WM_USER + 2100
#define IDM_SAVE		WM_USER + 2101
#define IDM_LOAD		WM_USER + 2102
#define IDM_GOALS		WM_USER + 2103
#define IDM_TASKS		WM_USER + 2104
#define IDM_PLAYER		WM_USER + 2105
#define IDM_CYCLE		WM_USER + 2106


// BUGBUG these values may change on info from Thielen
// general defines 
#define NUM_DIFFICUTY_LEVELS	4

#define MAX_GOALS			1000
#define MAX_TASKS			1000
#define MAX_OPFORS			64
#define MAX_TASKPARAMS		8
#define MAX_ADJACENT		8

// BUGBUG these values will change during playtesting
// task parameter range constants
#define MAX_RANGE			100
#define MED_RANGE			50
#define MIN_RANGE			10

// the number of initial goals to start with
#define NUM_INITIAL_GOALS	24
// number of points used to define a patrol
#define NUM_PATROL_POINTS	4

// set the threshholds for idle trucks
#define MINIMUM_IDLE_TRUCKS_AI	3
#define EXCESS_IDLE_MATERIALS	2000 // roughly a single truck load of anything

// types of data
#define TASK_DATA		1
#define GOAL_DATA		2

// goal types used by CAIGoal
#define TRADE_GOAL			(BYTE)1
#define INDUSTRY_GOAL		(BYTE)2
#define LAND_WARFARE_GOAL	(BYTE)3
#define NAVAL_WARFARE_GOAL	(BYTE)4

// task types used by CAITask
#define ORDER_TASK		(BYTE)1
#define INFO_TASK		(BYTE)2
#define MESSAGE_TASK	(BYTE)3
#define COMBAT_TASK		(BYTE)4

// task status codes used by CAITask
#define UNASSIGNED_TASK	(BYTE)0
#define INPROCESS_TASK	(BYTE)1
#define COMPLETED_TASK	(BYTE)2

// MESSAGE_TASK types used
// CAITask::m_pwaParams[] constants for offset use
#define MESSAGE_ID		0

// COMBAT_TASK types used
#define TASK_RANGE		0

// INFO_TASK types used
#define RESOURCE_ID		1
#define FIND_PRIORTY	2

// ORDER_TASK types used in CAITask::m_pwaParams[ORDER_TYPE]
#define CONSTRUCTION_ORDER	301
#define PRODUCTION_ORDER	302
#define TRANSPORT_ORDER		303

// CAITask::m_pwaParams[] constants for offset use
#define ORDER_TYPE			0	// type of order to issue

// CAITask::m_pwaParams[] used for PRODUCTION_ORDER
#define PRODUCTION_ITEM		1	// type id of material or vehicle
#define PRODUCTION_TYPE		2   // material or vehicle to produce
#define PRODUCTION_ID1		3	// id of primary factory type
#define PRODUCTION_ID2		4	// id of secondary factory type
#define PRODUCTION_QTY		5	// how many to build

// CAITask::m_pwaParams[] used for CONSTRUCTION_ORDER
#define BUILDING_ID		1	// type of building to build
#define BUILDING_EXIT	5	// orientation of exit of building
#define BUILD_AT_X		6	// updated when construction is
#define BUILD_AT_Y		7	// ordered and is site of building

// CAITask::m_pwaParams[] used for TRANSPORT_ORDER
#define FROM_MATERIAL_ID	1	// id of material in transport
#define FROM_X				2	// from 'from' site
#define FROM_Y				3
#define TO_MATERIAL_ID		4	// id of material in transport
#define TO_X				5	// from 'to' site
#define TO_Y				6


// COMBAT_TASK target types used in CAITask::m_pwaParams[1]
#define BEST_TARGET			310
#define NEAREST_TARGET		311
#define THREAT_TARGET		312
#define WEAKEST_TARGET		313

// CAIMap status word bitmap definitions
#define MSW_AI_BUILDING		0x0001	// bit 0
#define MSW_AI_VEHICLE		0x0002	// bit 1
#define MSW_OPFOR_BUILDING	0x0004	// bit 2
#define MSW_OPFOR_VEHICLE	0x0008	// bit 3
#define MSW_KNOWN			0x0010	// bit 4
#define MSW_ROAD			0x0020	// bit 5
#define MSW_PLANNED_ROAD	0x0040	// bit 6
#define MSW_RESOURCE		0x0080	// bit 7
#define MSW_STAGING			0x0100  // bit 8 - used to flag staging

// CAIMap main road layout types
#define CAI_CROSS_ROADS		(BYTE)1 // 2 roads forming a cross
#define CAI_1V_ROAD			(BYTE)2 // 3 roads, 1 vertical
#define CAI_2V_ROAD			(BYTE)3 // 3 roads, 2 vertical
#define CAI_TTT_ROAD		(BYTE)4 // 4 roads, tic-tac-toe style

// CAIMap power plant distribution
#define CAI_CENTRAL_PLANTS	(BYTE)1 // all adjacent
#define CAI_DISTRIB_PLANTS	(BYTE)2 // none adjacent

// CAIUnit param word array size and offset constants

//	ASSERT( CMaterialTypes::lumber == CAI_LUMBER );
//	ASSERT( CMaterialTypes::steel == CAI_STEEL );
//	ASSERT( CMaterialTypes::copper == CAI_COPPER );
//	ASSERT( CMaterialTypes::moly == CAI_MOLY );
//	ASSERT( CMaterialTypes::goods == CAI_GOODS );
//	ASSERT( CMaterialTypes::food == CAI_FOOD );
//	ASSERT( CMaterialTypes::oil == CAI_OIL );
//	ASSERT( CMaterialTypes::coal == CAI_COAL );
//	ASSERT( CMaterialTypes::iron == CAI_IRON );
//	ASSERT( CMaterialTypes::num_types == CAI_UNASSIGNED );
//	ASSERT( CMaterialTypes::num_types <= CAI_DATA_SLOTS );

//#define CAI_WHEAT	0
//#define CAI_TREES	2

/*
			enum { lumber,			// lumber - goods must be 0-based
						steel,
						copper,
						moly,
						goods,

						food,
						oil,
						gas,
						coal,
						iron,
						num_types,
						num_build_types = food };	// goods + 1
*/

// used to id the CAIUnit::m_pwaParams slots for materials
#define CAI_LUMBER	0
#define CAI_STEEL	1	// composite material
#define CAI_COPPER	2
#define CAI_MOLY	3
#define CAI_GOODS	4	// composite material
#define CAI_FOOD	5
#define CAI_OIL		6
#define CAI_GAS		7
#define CAI_COAL	8
#define CAI_IRON	9

#define CAI_ROUTE_X	12
#define CAI_ROUTE_Y	13

#define CAI_UNASSIGNED	10
#define CAI_SIZE		16	// many values depend on this being 16

// also used in CAIUnit::m_pwaParams slots for transporting materials
//#define FROM_MATERIAL_ID	1	// material transported
//#define FROM_X			2	// from 'from' site
//#define FROM_Y			3
#define FROM_ID_LO			4	// id of source building for materials
#define FROM_ID_HI			5

// BUGBUG this needs to be unique among player numbers
#define CAI_OPFOR_UNIT	0xFFFF

// CAIUnit and CAICopy object data identifiers
#define CAI_DATA_SLOTS	16
// used in CAICopy::m_aiDataIn[]
#define CAI_PEOPLE		0
#define CAI_MAXMATERIAL	1
#define CAI_SPOTTING	2
#define CAI_DAMAGEPTS	3
#define CAI_INITIATIVE	4
#define CAI_RANGE		5
#define CAI_SOFTATTACK	6
#define CAI_HARDATTACK	7
#define CAI_NAVALATTACK	8
#define CAI_TARGETTYPE	9
#define CAI_GRDDEFENSE	10	
#define CAI_PATROL		11	// used in CAIUnit::m_pwaParams
#define CAI_CLSDEFENSE	12
#define CAI_FIRERATE	13
#define CAI_ACCURACY	14
#define CAI_TYPEBUILD	15
#define CAI_TYPEVEHICLE	15

// used in CAICopy::m_aiDataOut[]
#define CAI_BUILDTYPE		14

#define CAI_LOC_X			0
#define CAI_LOC_Y			1
#define CAI_ISCONSTRUCTING	2
#define CAI_ISPAUSED		3
#define CAI_ISWAITING		4
#define CAI_PRODUCES		5	// must be less than CAI_DAMAGE

#define CAI_PREV_X			2
#define CAI_PREV_Y			3
#define CAI_DEST_X			4
#define CAI_DEST_Y			5
#define CAI_FUEL			6
#define CAI_DAMAGE			7
#define CAI_EFFECTIVE		8
#define CAI_AMMO			9	// must be less than CAI_UNASSIGNED

#define CAI_TIMEBUILD		CAI_UNASSIGNED


#define CAI_MORALE			0
#define CAI_POWER			1
#define CAI_NOPOWER			2
#define CAI_UNIONTYPE		3	// CStructureData::m_iType

// used by IDT_PREPAREWAR to contain counts of types of units
// and MUST be less than MAX_TASKPARAMS
#define CAI_TF_TANKS		4
#define CAI_TF_IFVS			5
#define CAI_TF_ARTILLERY	6
#define CAI_TF_INFANTRY		7

#define CAI_TF_CRUISERS		4
#define CAI_TF_DESTROYERS	5

#define CAI_TF_ARMOR		4
#define CAI_TF_LANDING		5
#define CAI_TF_SHIPS		6
#define CAI_TF_MARINES		7


// CAIUnit routing status word definitions for buildings
#define CAI_NEED_COMMODITY		0x0001	// a shortfall in commodity for building
#define CAI_COMMODITY_ENROUTE   0x0002  // the commodity needed is enroute
#define CAI_TRUCK_WAITING		0x0004  // there is a truck waiting at building
#define CAI_SUPPLIED			0x8000	// all commodities needed are present

// CAIUnit routing status word definitions for vehicles
#define CAI_IN_USE				0x0001	// this vehicle is on a mission
#define CAI_IN_COMBAT           0x0002  // this vehicle is in combat
#define CAI_TASKSWITCH			0x0004	// this unit's task was switched 
										// old task saved in CAI_UNASSIGNED
#define CAI_IS_CARGO			0x0008  // this vehicle is on a cargo ship

/////////////////////////////////////////////////////////////////////////////


// PRIMARY GOALS

#define IDG_ESTABLISH		1000	// establish colony
#define IDG_BASICFEED		1001	// provide for feeding the people
#define IDG_BASICPOWER		1002	// provide basic power needs
#define IDG_BASICPRODUCTION	1003	// provide basic production
#define IDG_BASICDEFENSE	1004	// provide basic defense
#define IDG_BASICEXPLORE	1005	// peform recon of area
#define IDG_BASICMOVE		1006	// establish basic roads and transports
#define IDG_BALANCE			1007	// balance production with resources
#define IDG_HOUSE			1008	// produce housing for people
#define IDG_DIPLOMACY		1009	// find other players and cities
#define IDG_TRADE			1010	// find, offer and assign trade
#define IDG_AGREEMENT		1011	// select player, offer agreement
#define IDG_CONQUER			1012	// conquer player
#define IDG_ADVFEED			1013	// produce extra food
#define IDG_ADVPOWER		1014	// produce excessive power
#define IDG_MORALE			1015	// improve morale of people
#define IDG_ADVMOVE			1016	// produce improved roads and transport
#define IDG_ADVPRODUCTION	1017	// produce advanced production facilities
#define IDG_LANDWAR			1018	// produce for land war
#define IDG_SEAWAR			1019	// produce for sea war
#define IDG_DEFENDATTACK	1020	// defend from attack
#define IDG_ESPIONAGE		1021	// conduct espionage against OPFORs
#define IDG_ADVDEFENSE		1022	// advanced defense tasks
#define IDG_SCOUTS			1023	// make scouts
#define IDG_LTANK			1024	// make tanks
#define IDG_MTANK			1025
#define IDG_HTANK			1026
#define IDG_LARTILLERY		1027	// make artillery
#define IDG_MARTILLERY		1028
#define IDG_HARTILLERY		1029
#define IDG_TROOPS			1030	// make infantry,rangers,marines
#define IDG_REPELL			1031	// repell seaborne invasion
#define IDG_SHORES			1032	// basic sea patrol of coast
#define IDG_SEAINVADE		1033	// build for amphib assault
#define IDG_PIRATE			1034	// harrass seaborne cargo
#define IDG_STDDEFENSE		1035	// standard defense
#define IDG_CONSTRUCTION	1036	// provide for construction
#define IDG_EMERPOWER		1037	// provide emergency power

#define IDG_ADDLUMBER		1050
#define IDG_ADDSTEEL		1051
#define IDG_ADDCOPPER		1052
#define IDG_ADDMOLY			1053
#define IDG_ADDGOODS		1054
#define IDG_ADDFARM			1055
#define IDG_ADDOILWELL		1056
#define IDG_ADDREFINERY		1057
#define IDG_ADDCOAL			1058
#define IDG_ADDIRON			1059
#define IDG_ADDTRUCKS		1060
#define IDG_BRIDGING		1061
#define IDG_CARGOSHIP		1062
#define IDG_GUNBOAT			1063
#define IDG_DESTROYER		1064
#define IDG_CRUISER			1065
#define IDG_BATTLESHIP		1066
#define IDG_LANDCRAFT		1067
#define IDG_IFV				1068

#define IDG_PATROLCITY		1070	// patrol close to city bounds
#define IDG_PATROLBORDER	1071	// patrol away from city
#define IDG_PATROLWORLD		1072	// patrol near opfor city
#define IDG_PATROLBLDG		1073	// patrol close to a building

// TASKS FROM GOALS

#define IDT_BUILDAPTS		2001	// Build Apartments
#define IDT_BUILDBARRACKS2	2002	// 20 - Build Barracks
#define IDT_BUILDCCC		2003	// 18 - Build Command Center
#define IDT_BUILDEMBASSY	2004	// 10 - Build Embassy
#define IDT_BUILDEMERGREP	2005	// Build Emergency Response
#define IDT_BUILDFARM		2006	// 30 - Build Farm
#define IDT_BUILDOFFICE	    2007	// Build Office
#define IDT_BUILDFORT1		2008	// 10 - Build Fort
#define IDT_BUILDGOODSFAC	2009	// 30 - Build Goods Factory
#define IDT_BUILDHEAVYFAC	2010	// 28 - Build Heavy Weapons Factory
#define IDT_BUILDLIGHT0 	2011	// 28 - Build Light Weapons Factory
#define IDT_BUILDLOGCAMP	2012	// Build Logging Camp
#define IDT_BUILDLUMBMILL	2013	// 30 - Build Lumber Mill
#define IDT_BUILDCOALMINE	2014	// 40 - Build Mine
#define IDT_BUILDIRONMINE	2015	// 40 - Build Mine
#define IDT_BUILDCOPPERMINE	2016	// 36 - Build Mine
#define IDT_BUILDMOLYMINE	2017	// 36 - Build Mine

#define IDT_BUILDPOWERPT1	2018	// 40 - Build Power Plant
#define IDT_BUILDRD			2019	// 40 - Build R&D Facility
#define IDT_BUILDRECCNTR	2020	// Build Recreation Center
#define IDT_BUILDREPAIRFAC	2021	// 30 - Build Repair Facility
#define IDT_BUILDSEAPORT	2022	// 28 - Build Seaport
#define IDT_BUILDSHIPYARD1	2023	// 28 - Build Shipyard
#define IDT_BUILDSMELTER	2024	// 30 - Build Smelter
#define IDT_BUILDMARKET		2025	// Build Supermarket
#define IDT_BUILDWAREHOUSE	2026	// 26 - Build Warehouse

#define IDT_BUILDOILWELL	2027	// 34 - Build Oil Well
#define IDT_BUILDREFINERY	2028	// 34 - Build Refinery

#define IDT_BUILDPOWERPT2	2029	// Build Power Plant 2
#define IDT_BUILDPOWERPT3	2030	// Build Power Plant 3
#define IDT_BUILDBARRACKS3	2031	// Build Barracks
#define IDT_BUILDFORT2		2032
#define IDT_BUILDFORT3		2033
#define IDT_BUILDLIGHT1		2034
#define IDT_BUILDLIGHT2		2035
#define IDT_BUILDSHIPYARD3  2036

#define IDT_MAKEWHEAT		2100	// Make Wheat
#define IDT_MAKEFOOD		2102	// Make Food
#define IDT_CUTTREES		2103	// Cut Trees
#define IDT_MAKELUMBER		2104	// Make Lumber
#define IDT_MINEOIL			2105	// 32 - Mine Oil
#define IDT_MINECOAL		2106	// 38 - Mine Coal
#define IDT_MINEIRON		2107	// 38 - Mine Iron
#define IDT_MAKESTEEL		2108	// 28 - Make Steel
#define IDT_MINECOPPER		2109	// 34 - Mine Copper
#define IDT_MINEMOLY		2110	// 34 - Mine Moly
#define IDT_MAKESUPPLIES	2111	// 28 - Make Supplies
#define IDT_MAKEPOWER		2112	// 38 - Make Power
#define IDT_RESEARCH		2113	// 38 - Do Research

#define IDT_MAKEINFANTRY	2200	// 18 - Make Infantry
#define IDT_MAKERANGERS		2201	// 18 - Make Rangers
#define IDT_MAKEMARINES		2202	// 18 - Make Marines
#define IDT_MAKECONSTRUCT	2203	// 26 - Make Construction truck
#define IDT_MAKELTRUCK		2204	// 26 - Make Light Truck
#define IDT_MAKEMTRUCK		2205	// 26 - Make Medium Truck
#define IDT_MAKEHTRUCK		2206	// 26 - Make Heavy Truck
#define IDT_MAKELSCOUT		2207	// 24 - Make Light Scout
#define IDT_MAKEMSCOUT		2208	// 24 - Make Medium Scout
#define IDT_MAKEHSCOUT		2209	// 24 - Make Heavy Scout
#define IDT_MAKEIFV			2210	// 22 - Make Infantry Carrier
#define IDT_MAKELTANK		2211	// 20 - Make Light Tank
#define IDT_MAKEMTANK		2212	// 20 - Make Medium Tank
#define IDT_MAKEHTANK		2213	// 20 - Make Heavy Tank
#define IDT_MAKELART		2214	// 20 - Make Light Artillery
#define IDT_MAKEMART		2215	// 20 - Make Medium Artillery
#define IDT_MAKEHART		2216	// 20 - Make Heavy Artillery
#define IDT_MAKEBRIDTRUCK	2217	// 18 - Make Bridging Truck
#define IDT_MAKELCARGOSHIP	2218	// 26 - Make Light Cargo Ship
#define IDT_MAKEMCARGOSHIP	2219	// 26 - Make Medium Cargo Ship
#define IDT_MAKEHCARGOSHIP	2220	// 26 - Make Heavy Cargo Ship
#define IDT_MAKEGUNSHIP		2221	// 24 - Make Gunboat Ship
#define IDT_MAKEDESTROYER	2222	// 24 - Make Destroyer
#define IDT_MAKECRUISER		2223	// 22 - Make Cruiser
#define IDT_MAKEBATTLESHIP	2224	// 22 - Make Battleship
#define IDT_MAKELANDCRAFT	2225	// 20 - Make Landing Craft

//#define IDT_MAKESPY			2226	// Build SPY

#define IDT_EVALUATE		2300	// 30 - Evaluate land
#define IDT_ATTACKUNIT		2301	// Identify shortfalls
#define IDT_SETTRANSPORT	2302	// Set Transport
#define IDT_RECONOPFOR		2303	// 20 - Recon for OpFors
#define IDT_FINDCOPPER		2304	// 20 - Find Copper
#define IDT_FINDCOAL		2305	// 20 - Find Coal
#define IDT_FINDIRON		2306	// 20 - Find Iron
#define IDT_FINDMOLY		2307	// 20 - Find Moly
#define IDT_FINDOIL			2308	// 20 - Find Oil
#define IDT_FINDOPFORS		2309	// 24 - Find OpFors
#define IDT_FINDOPFORCITY	2310	// 24 - Find OpFor cities
#define IDT_FINDOPFORAI		2311	// 26 - Find AI OpFors
#define IDT_FINDOPFORTRADE	2312	// 24 - Find Trade OpFor
#define IDT_OFFERTRADE		2320	// Offer Trade
#define IDT_ASSIGNTRADE		2321	// Assign Trade Route
#define IDT_SELECTAGREE		2322	// Select Agreement
#define IDT_OFFERAGREE		2323	// Offer Agreement
#define IDT_ACCEPTAGREE		2324	// Accept Agreement
#define IDT_PREPAREWAR		2325	// Prepare for War
#define IDT_BREAKAGREE		2326	// Break Agreements
#define IDT_SEEKINRANGE		2327	// seek and destory in range
#define IDT_SEEKINWAR		2328	// seek and destory in war
#define IDT_SEEKATSEA		2329	// seek and destory at sea
#define IDT_CONDUCTWAR		2330	// conduct war
#define IDT_PATROL			2331	// Patrol Range	
#define IDT_GETINTEL		2332	// Gather Intelligence
#define IDT_STEALTECH		2333	// Steal Technology
#define IDT_SABOTAGE		2334	// Conduct Sabotage
#define IDT_SCOUT0			2335	// 40 - Scout Block0
#define IDT_SCOUT1			2336	// 40 - Scout Block1
#define IDT_SCOUT2			2337	// 40 - Scout Block2
#define IDT_SCOUT3			2338	// 40 - Scout Block3
#define IDT_SCOUT			2339	// 40 - Scout Blocks
#define IDT_ESCORT			2340	// 60 - escort a convoy of trucks
#define IDT_CONSTRUCT		2341	// construct a building/road/bridge
#define IDT_REPAIR			2342	// repair a building/road/bridge
#define IDT_REPAIRING		2343	// vehicle being repaired

#ifndef __CAI_H__
#define __CAI_H__

// structures used for save/load

// CAIHex
typedef struct caihex_buffer {
	int iX;
	int iY;
	int iUnit;
	DWORD dwUnitID;
	BYTE cTerrain;
} CAIHexBuff;

// CAIUnit
typedef struct caiunit_buffer {
	DWORD dwID;
	int iOwner;
	int iType;		// value of CUnit::building
						// or CUnit::vehicle
	int iTypeUnit;	// CStructureData or CTransportData id

	BOOL bControl;	// indicates if unit is controled
						// by the AI, FALSE=HP unit
	WORD wGoal;		// current goal of this unit
	WORD wTask;		// current task of this unit
	DWORD dwData;		// possible id of building/opfor attacking

	WORD wStatus;		// a status flag to use generally
	int iParams[CAI_SIZE];
	DWORD dwParams[CAI_SIZE];
} UnitBuff;

// CAIMsg
typedef struct caimsg_buffer {
	int	iMsg;			// id of message
	int iPriority;	// priority of this message
	unsigned uFlags;	// used for status and flags
	DWORD dwID;		// id of CUnit if applicable
	int	iX;			// used for location
	int	iY;
	int	ieX;			// used for destination
	int	ieY;
	int idata1;		// used for m_iType, m_iStatus, 
						// m_iBldg, m_iVeh, m_iDamage
	int idata2;		// used for m_iNum, m_bOK
	int idata3;
	DWORD dwID2;		// id of 2nd CUnit if needed
} MsgBuff;


// CAIOpFor
typedef struct opfor_buffer {
	int iPlayer;
	DWORD dwRocket;
	int iMsgCount;
	int iAttitude;
	BYTE cRelations;
	BOOL bAtWar;
	BOOL bIsAI;
	BOOL bIsKnown;
} OpForBuff;

// these structures support the binary i/o

//  CAIGoal( WORD wID, BYTE cType, CWordArray *pwaTasks );
typedef struct goal_buffer {
	int iID;
	int iType;
	int iTasks[NUM_INITIAL_GOALS];
} GoalBuff;

//	CAITask( WORD wID, BYTE cType, BYTE cPriority, 
//		WORD wOrderID, CWordArray *pwaParams );

typedef struct task_buffer {
	int iID;
	int iGoal;
	int iType;
	int iPriority;
	int iOrderID;
	int iParams[MAX_TASKPARAMS];
} TaskBuff;

#endif // __CAI_H__

// end of cai.h
