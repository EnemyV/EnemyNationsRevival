//---------------------------------------------------------------------------
// wire_layout_assert.cpp — compile-time size pins for the network wire structs.
//
// Cross-platform MP (Release 3) sends the CNetCmd/CMsg structs in netcmd.h
// byte-for-byte over the socket. They are wrapped in `#pragma pack(...,1)`,
// use only fixed-width members (DWORD/int/signed char + int-sized enums), and
// contain no `long`/`time_t`/`size_t` — so their RELEASE layout is identical
// across MSVC/gcc/clang on x86-64 and ARM64 (all little-endian).
//
// IMPORTANT — Debug vs Release: several wire structs embed CSubHex/CHexCoord/
// CObject-derived members that declare `virtual void AssertValid()` under
// `#ifdef _DEBUG`. In a _DEBUG build those structs become POLYMORPHIC and gain
// an 8-byte vtable pointer, so their Debug sizeof differs from Release (e.g.
// _CMsgVeh = 60 release / 76 debug, via two CSubHex members at +8 each).
// The wire format that actually ships and goes over the socket is the RELEASE
// (NDEBUG) layout, and MP only runs in Release — so we pin the RELEASE sizes
// and only assert when !_DEBUG. (Caveat for the record: do NOT run MP from a
// _DEBUG build — the Debug vtable pointer is not part of the wire format.)
//
// These asserts catch a future `long`/pointer/`time_t` add or repad to a wire
// struct as a build break on ALL platforms instead of a silent cross-platform
// desync. Sizes are the Release (NDEBUG) pack(1) canonical values. Update only
// with a deliberate, lead+human-approved wire-format change.
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "netcmd.h"

#ifndef _DEBUG   // wire format = Release layout; Debug adds vtables (see header)
static_assert(sizeof(CNetCmd)==12, "CNetCmd");
static_assert(sizeof(CNetReady)==140, "CNetReady");
static_assert(sizeof(CNetPlyrJoin)==93, "CNetPlyrJoin");
static_assert(sizeof(CNetEnumPlyrs)==16, "CNetEnumPlyrs");
static_assert(sizeof(CNetSelectPlyr)==100, "CNetSelectPlyr");
static_assert(sizeof(CNetYouAre)==20, "CNetYouAre");
static_assert(sizeof(CNetPlayer)==152, "CNetPlayer");
static_assert(sizeof(CNetStart)==48, "CNetStart");
static_assert(sizeof(CNetPlyrStatus)==20, "CNetPlyrStatus");
static_assert(sizeof(CNetInitDone)==16, "CNetInitDone");
static_assert(sizeof(CNetChat)==21, "CNetChat");
static_assert(sizeof(CNetPlay)==16, "CNetPlay");
static_assert(sizeof(CNetToAi)==16, "CNetToAi");
static_assert(sizeof(CNetToHp)==21, "CNetToHp");
static_assert(sizeof(CNetGetFile)==28, "CNetGetFile");
static_assert(sizeof(_CMsgVeh)==60, "_CMsgVeh");
static_assert(sizeof(_CMsgBldg)==36, "_CMsgBldg");
static_assert(sizeof(_CMsgRoad)==32, "_CMsgRoad");
static_assert(sizeof(_CMsgBridge)==48, "_CMsgBridge");
static_assert(sizeof(_CMsgVehGo)==116, "_CMsgVehGo");
static_assert(sizeof(CMsgUnitDamage)==40, "CMsgUnitDamage");
static_assert(sizeof(CMsgUnitSetDamage)==24, "CMsgUnitSetDamage");
static_assert(sizeof(CMsgPlaceBldg)==36, "CMsgPlaceBldg");
static_assert(sizeof(CMsgPlaceVeh)==60, "CMsgPlaceVeh");
static_assert(sizeof(CMsgBuildBldg)==36, "CMsgBuildBldg");
static_assert(sizeof(CMsgBuildVeh)==24, "CMsgBuildVeh");
static_assert(sizeof(CMsgBuildRoad)==32, "CMsgBuildRoad");
static_assert(sizeof(CMsgBuildBridge)==48, "CMsgBuildBridge");
static_assert(sizeof(CMsgVehGoto)==116, "CMsgVehGoto");
static_assert(sizeof(CMsgTransMat)==60, "CMsgTransMat");
static_assert(sizeof(CMsgUnitControl)==17, "CMsgUnitControl");
static_assert(sizeof(CMsgUnitRepair)==28, "CMsgUnitRepair");
static_assert(sizeof(CMsgDestroyUnit)==16, "CMsgDestroyUnit");
static_assert(sizeof(CMsgBldgNew)==36, "CMsgBldgNew");
static_assert(sizeof(CMsgBldgStat)==48, "CMsgBldgStat");
static_assert(sizeof(CMsgVehNew)==60, "CMsgVehNew");
static_assert(sizeof(CMsgVehStat)==32, "CMsgVehStat");
static_assert(sizeof(CMsgVehLoc)==116, "CMsgVehLoc");
static_assert(sizeof(CMsgVehDest)==28, "CMsgVehDest");
static_assert(sizeof(CMsgRoadNew)==32, "CMsgRoadNew");
static_assert(sizeof(CMsgRoadDone)==32, "CMsgRoadDone");
static_assert(sizeof(CMsgBridgeNew)==48, "CMsgBridgeNew");
static_assert(sizeof(CMsgBridgeDone)==48, "CMsgBridgeDone");
static_assert(sizeof(CMsgVehSetDest)==40, "CMsgVehSetDest");
static_assert(sizeof(CMsgUnitDying)==24, "CMsgUnitDying");
static_assert(sizeof(CMsgDeleteUnit)==24, "CMsgDeleteUnit");
static_assert(sizeof(CMsgAttack)==20, "CMsgAttack");
static_assert(sizeof(CMsgDeployIt)==16, "CMsgDeployIt");
static_assert(sizeof(CMsgPlyrDying)==16, "CMsgPlyrDying");
static_assert(sizeof(CMsgPlyrDead)==20, "CMsgPlyrDead");
static_assert(sizeof(CMsgRsrch)==20, "CMsgRsrch");
static_assert(sizeof(CMsgLoadCarrier)==20, "CMsgLoadCarrier");
static_assert(sizeof(CMsgUnloadCarrier)==16, "CMsgUnloadCarrier");
static_assert(sizeof(CMsgUnitAttacked)==44, "CMsgUnitAttacked");
static_assert(sizeof(CMsgBuildCiv)==20, "CMsgBuildCiv");
static_assert(sizeof(CMsgSeeUnit)==44, "CMsgSeeUnit");
static_assert(sizeof(CMsgRepairVeh)==16, "CMsgRepairVeh");
static_assert(sizeof(CMsgRepairBldg)==20, "CMsgRepairBldg");
static_assert(sizeof(CMsgScenario)==24, "CMsgScenario");
static_assert(sizeof(CMsgScenarioAtk)==32, "CMsgScenarioAtk");
static_assert(sizeof(CMsgLoaded)==20, "CMsgLoaded");
static_assert(sizeof(CMsgRepaired)==20, "CMsgRepaired");
static_assert(sizeof(CMsgAiMsg)==24, "CMsgAiMsg");
static_assert(sizeof(CMsgOutOfLos)==28, "CMsgOutOfLos");
static_assert(sizeof(CMsgMatChange)==20, "CMsgMatChange");
static_assert(sizeof(CMsgSetRelations)==24, "CMsgSetRelations");
static_assert(sizeof(CMsgBldgMat)==60, "CMsgBldgMat");
static_assert(sizeof(CMsgGameSpeed)==20, "CMsgGameSpeed");
static_assert(sizeof(CMsgNewServer)==16, "CMsgNewServer");
static_assert(sizeof(CMsgGiveUnit)==60, "CMsgGiveUnit");
static_assert(sizeof(CMsgSetTime)==16, "CMsgSetTime");
static_assert(sizeof(CMsgStartFile)==16, "CMsgStartFile");
static_assert(sizeof(CMsgStartLoadedGame)==12, "CMsgStartLoadedGame");
static_assert(sizeof(CMsgCancelLoad)==16, "CMsgCancelLoad");
static_assert(sizeof(CMsgPauseMsg)==17, "CMsgPauseMsg");
static_assert(sizeof(CMsgVehCompLoc)==700, "CMsgVehCompLoc");
static_assert(sizeof(CMsgCompUnitDamage)==696, "CMsgCompUnitDamage");
static_assert(sizeof(CMsgCompUnitSetDamage)==696, "CMsgCompUnitSetDamage");
static_assert(sizeof(CMsgShoot)==688, "CMsgShoot");
static_assert(sizeof(CNetAiGpf)==16, "CNetAiGpf");
static_assert(sizeof(CNetRsrchDisc)==20, "CNetRsrchDisc");
static_assert(sizeof(CNetNeedSaveInfo)==16, "CNetNeedSaveInfo");
static_assert(sizeof(CNetSaveInfo)==40, "CNetSaveInfo");
#endif // !_DEBUG
