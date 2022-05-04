//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// sprite.cpp : the sprites
//

#include "stdafx.h"
#include "lastplnt.h"
#include "sprite.h"
#include "help.h"
#include "error.h"
#include "sfx.h"

#include "terrain.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

MEM_POOL _memPoolCompSprites = NULL;
MEM_POOL _memPoolSprites = NULL;

int * * CVehicle::m_apiWid = NULL;
int CVehicle::m_iMaxRange = 0;

int CTransportData::m_iMaxDraft = CHex::sea_level;
CString	CTransportData::m_sAuto;
CString	CTransportData::m_sRoute;
CString	CTransportData::m_sIdle;
CString	CTransportData::m_sTravel;

CCriticalSection	CSpriteCollection::g_cs;
BOOL					CSpriteCollection::g_bTerminateReadThread;
BOOL					CSpriteCollection::g_bReadThreadError;

CList< Ptr< CSpriteParms >, Ptr< CSpriteParms > & >	CSpriteCollection::g_listptrspriteparms;

const unsigned TERRAIN_TIME  = 6;
const unsigned BUILDING_TIME = 3;
const unsigned VEHICLE_TIME  = 4;
const unsigned EFFECT_TIME   = 2;
const unsigned FARM_TIME  	  = 120;

/////////////////////////////////////////////////////////////////////////////
// CTerrain initialization

CTerrain::~CTerrain () 
{
}

void CTerrain::InitData ()
{

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x01 )
		return;

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ("units", "UNIT");

	ptrMmio->DescendRiff ('U', 'N', 'I', 'T');
	ptrMmio->DescendList ('T', 'E', 'R', 'N');

	ASSERT_STRICT (NUM_WHEEL_TYPES == 5);
	CTerrainData * pTd = m_Data;
	for (int iOn=0; iOn<CHex::num_types; iOn++, pTd++)
		{
		theApp.BaseYield ();

		ptrMmio->DescendChunk ('D', 'A', 'T', 'A');
		pTd->m_iBuildMult = ptrMmio->ReadShort ();
		pTd->m_iFarmMult = ptrMmio->ReadShort ();
		for (int n=0; n<=4; n++)
			pTd->m_iWheelMult[n] = ptrMmio->ReadShort ();
		for (n=0; n<=4; n++)
			pTd->m_iDefenseMult[n] = ptrMmio->ReadShort ();
		ptrMmio->AscendChunk ();
		}

	ptrMmio->AscendList ();

	m_bLoaded |= 0x01;

	theApp.BaseYield ();
}

void CTerrain::InitSprites ()
{
	theApp.m_pCreateGame->GetDlgStatus()->SetPer (PER_TERRAIN_INIT);

	// see if loaded
	if ( m_bLoaded & 0x02 )
		return;

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ( GetRifName(), "TERN" );

	ptrMmio->DescendRiff ('T', 'E', 'R', 'N');

	Read(*ptrMmio, TERRAIN_TIME, PER_TERRAIN_INIT, PER_NUM_TERRAIN_INIT, ptrthebltformat->GetBitsPerPixel() );

	// rotated roads
	MakeRotated ( CHex::road, 0, CHex::road, 1, 1 );
	MakeRotated ( CHex::road, 2, CHex::road, 3, 1 );
	MakeRotated ( CHex::road, 2, CHex::road, 4, 2 );
	MakeRotated ( CHex::road, 2, CHex::road, 5, 3 );
	MakeRotated ( CHex::road, 6, CHex::road, 7, 1 );
	MakeRotated ( CHex::road, 6, CHex::road, 8, 2 );
	MakeRotated ( CHex::road, 6, CHex::road, 9, 3 );

	// rotated coastline
	MakeRotated ( CHex::coastline, 0, CHex::coastline, 1, 1 );
	MakeRotated ( CHex::coastline, 0, CHex::coastline, 2, 2 );
	MakeRotated ( CHex::coastline, 0, CHex::coastline, 3, 3 );
	MakeRotated ( CHex::coastline, 4, CHex::coastline, 5, 1 );
	MakeRotated ( CHex::coastline, 4, CHex::coastline, 6, 2 );
	MakeRotated ( CHex::coastline, 4, CHex::coastline, 7, 3 );
	MakeRotated ( CHex::coastline, 8, CHex::coastline, 9, 1 );
	MakeRotated ( CHex::coastline, 8, CHex::coastline, 10, 2 );
	MakeRotated ( CHex::coastline, 8, CHex::coastline, 11, 3 );

	MakeRotated ( CHex::coastline, 13, CHex::coastline, 14, 1 );
	MakeRotated ( CHex::coastline, 13, CHex::coastline, 15, 2 );
	MakeRotated ( CHex::coastline, 13, CHex::coastline, 16, 3 );
	MakeRotated ( CHex::coastline, 17, CHex::coastline, 18, 1 );
	MakeRotated ( CHex::coastline, 17, CHex::coastline, 19, 2 );
	MakeRotated ( CHex::coastline, 17, CHex::coastline, 20, 3 );
	MakeRotated ( CHex::coastline, 21, CHex::coastline, 22, 1 );
	MakeRotated ( CHex::coastline, 21, CHex::coastline, 23, 2 );
	MakeRotated ( CHex::coastline, 21, CHex::coastline, 24, 3 );

	MakeRotated ( CHex::coastline, 26, CHex::coastline, 27, 1 );
	MakeRotated ( CHex::coastline, 26, CHex::coastline, 28, 2 );
	MakeRotated ( CHex::coastline, 26, CHex::coastline, 29, 3 );
	MakeRotated ( CHex::coastline, 30, CHex::coastline, 31, 1 );
	MakeRotated ( CHex::coastline, 30, CHex::coastline, 32, 2 );
	MakeRotated ( CHex::coastline, 30, CHex::coastline, 33, 3 );
	MakeRotated ( CHex::coastline, 34, CHex::coastline, 35, 1 );
	MakeRotated ( CHex::coastline, 34, CHex::coastline, 36, 2 );
	MakeRotated ( CHex::coastline, 34, CHex::coastline, 37, 3 );

	m_bLoaded |= 0x02;

	theApp.BaseYield ();

	#ifdef _DEBUG
	ASSERT_VALID (this);
	for ( int i = 0; i < GetCount(); ++i )
		GetSpriteByIndex( i )->CheckValid();
	#endif
}

//---------------------------------------------------------------------------
// CTerrain::MakeRotated
//---------------------------------------------------------------------------
void
CTerrain::MakeRotated(
	int	iIDSrc,
	int	iIndexSrc,
	int	iIDDst,
	int	iIndexDst,
	int	iRotRight )
{
	CTerrainSprite	* pterrainsprite = GetSprite( iIDSrc, iIndexSrc, TRUE );

	if ( !pterrainsprite )
		ThrowError( ERR_TLP_BAD_DATA );

	theApp.BaseYield ();

	Ptr< CSprite >	ptrsprite = new CTerrainSprite( *pterrainsprite, iIDDst, iIndexDst, iRotRight );

	SetSprite( ptrsprite, iIDDst, iIndexDst );
}

void CTerrain::InitLang ()
{

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x04 )
		return;

	// read in the text
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO (NULL, "LANG");

	ptrMmio->DescendRiff ('L', 'A', 'N', 'G');
	ptrMmio->DescendList ('T', 'E', 'R', 'N');

	CTerrainData * pTd = m_Data;
	for (int iOn=0; iOn<CHex::num_types; iOn++, pTd++)
		{
		ptrMmio->DescendChunk ('L', 'I', 'N', 'E');
		ptrMmio->ReadString (pTd->m_sDesc);
		ptrMmio->AscendChunk ();
		}
	ptrMmio->AscendList ();

	m_bLoaded |= 0x04;

	theApp.BaseYield ();

	ASSERT_STRICT_VALID (this);
}

/////////////////////////////////////////////////////////////////////////////
// CExplGrp - explosion groups

void CExplData::InitData (CMmio * pMmio)
{

	pMmio->DescendChunk ('D', 'A', 'T', 'A');

	m_pProjSprite = (CSprite *) pMmio->ReadShort ();
	m_iProjDelay = pMmio->ReadShort () * AVG_SPEED_MUL;

	m_pExpSprite = (CSprite *) pMmio->ReadShort ();
	m_iExplSound = pMmio->ReadShort ();
	if (m_iExplSound > 0)
		m_iExplSound += SOUNDS::explosion_base - 1;

	m_iFlags = pMmio->ReadShort ();

	pMmio->AscendChunk ();
}

void CExplData::PostSpriteInit ()
{

	if ((int) m_pProjSprite == -1)
		m_pProjSprite = NULL;
	else
		m_pProjSprite = theEffects.GetSprite (CEffect::projectile, (int) m_pProjSprite);

	if ((int) m_pExpSprite == -1)
		m_pExpSprite = NULL;
	else
		m_pExpSprite = theEffects.GetSprite (CEffect::explosion, (int) m_pExpSprite);
}

void CExplGrp::InitData ()
{

	theApp.BaseYield ();

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ("units", "UNIT");

	ptrMmio->DescendRiff ('U', 'N', 'I', 'T');
	ptrMmio->DescendList ('E', 'X', 'P', 'L');

	ptrMmio->DescendChunk ('N', 'U', 'M', 'E');
	m_iNumExpl = ptrMmio->ReadShort ();
	ASSERT_STRICT (m_iNumExpl == NUM_EXPL_DATA);

	if ( m_iNumExpl > NUM_EXPL_DATA )
		ThrowError( ERR_TLP_BAD_DATA );

	ptrMmio->AscendChunk ();

	for (int iOn=0; iOn<m_iNumExpl; iOn++)
		m_Data[iOn].InitData (ptrMmio.Value());

	theApp.BaseYield ();
}

void CExplGrp::PostSpriteInit ()
{

	for (int iOn=0; iOn<m_iNumExpl; iOn++)
		m_Data[iOn].PostSpriteInit ();

	theApp.BaseYield ();
}


/////////////////////////////////////////////////////////////////////////////
// CStructure - initialization

		// CUnitData stuff
void CUnitData::ReadUnitData (CMmio & mmio)
{

	theApp.BaseYield ();

	for (int iOn=0; iOn<4; iOn++)
		m_iRsrchReq[iOn] = mmio.ReadShort ();
	m_iScenario = mmio.ReadShort ();
	m_iPeople = mmio.ReadShort ();
	m_iMaxMaterials = mmio.ReadShort ();
	m_iSpottingRange = mmio.ReadShort ();
	m_iSpottingRange = __min ( m_iSpottingRange, 10 );
	m_iDamagePoints = mmio.ReadShort ();
	m_udFlags = (CUnitData::UNIT_DATA_FLAGS) mmio.ReadShort ();
	m_iRange = mmio.ReadShort ();
	m_iRange = __min ( m_iRange, 10 );
	m_iAttack [0] = mmio.ReadShort ();
	m_iAttack [1] = mmio.ReadShort ();
	m_iAttack [2] = mmio.ReadShort ();
	m_iExpRadius = mmio.ReadShort ();
	m_iTargetType = mmio.ReadShort ();
	m_iDefense = mmio.ReadShort ();
	m_iFireRate = mmio.ReadShort ();
	m_iAccuracy = mmio.ReadShort ();
	m_iSoundIdle = mmio.ReadShort ();
	m_iSoundGo = mmio.ReadShort ();
	m_iSoundRun = mmio.ReadShort ();
	m_iSoundShoot = mmio.ReadShort ();
	if (m_iSoundShoot > 0)
		m_iSoundShoot += SOUNDS::shoot_base - 1;
	m_dwIDProj = mmio.ReadShort ();
}

void CStructure::InitData ()
{

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x01 )
		return;

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ("units", "UNIT");

	ptrMmio->DescendRiff ('U', 'N', 'I', 'T');
	ptrMmio->DescendList ('B', 'L', 'D', 'G');

	ptrMmio->DescendChunk ('N', 'U', 'M', 'B');
	m_iNumBuildings = ptrMmio->ReadShort ();
	ptrMmio->AscendChunk ();
	ASSERT_STRICT (m_ppData == NULL);
	m_ppData = new CStructureData * [m_iNumBuildings];
	memset (m_ppData, 0, sizeof(CStructureData *) * m_iNumBuildings);

	CStructureData * * ppSd = m_ppData;
	for (int iOn=0; iOn<theStructures.GetNumBuildings (); iOn++, ppSd++)
		{
		theApp.BaseYield ();

		ptrMmio->DescendChunk ('D', 'A', 'T', 'A');

		// CBuildUnion
		int iType = ptrMmio->ReadShort ();
		switch (iType)
		  {
			case CStructureData::UTmaterials :
				*ppSd = new CBuildMaterials ();
				break;
			case CStructureData::UTvehicle :
				*ppSd = new CBuildVehicle ();
				break;
			case CStructureData::UThousing :
				*ppSd = new CBuildHousing ();
				break;
			case CStructureData::UTcommand :
				*ppSd = new CBuildCommand ();
				break;
			case CStructureData::UTembassy :
				*ppSd = new CBuildEmbassy ();
				break;
			case CStructureData::UTfort :
				*ppSd = new CBuildFort ();
				break;
			case CStructureData::UTpower :
				*ppSd = new CBuildPower ();
				break;
			case CStructureData::UTresearch :
				*ppSd = new CBuildResearch ();
				break;
			case CStructureData::UTrepair :
				*ppSd = new CBuildRepair ();
				break;
			case CStructureData::UTwarehouse :
				*ppSd = new CBuildWarehouse ();
				break;
			case CStructureData::UTmine :
				*ppSd = new CBuildMine ();
				break;
			case CStructureData::UTfarm :
				*ppSd = new CBuildFarm ();
				break;
			case CStructureData::UTshipyard :
				*ppSd = new CBuildShipyard ();
				break;
			default:
				ASSERT_STRICT (FALSE);
				ThrowError (ERR_TLP_BAD_DATA);
				break;
		  }

		// CUnitData stuff
		(*ppSd)->m_iType = (CStructureData::BLDG_TYPE) iOn;
		(*ppSd)->ReadUnitData (*ptrMmio);

		// BUGBUG - inc building points
		(*ppSd)->m_iDamagePoints += (*ppSd)->m_iDamagePoints / 2;

		if ((*ppSd)->m_iSoundIdle > 0)
			(*ppSd)->m_iSoundIdle += SOUNDS::bld_base - 1;
		if ((*ppSd)->m_iSoundRun > 0)
			(*ppSd)->m_iSoundRun += SOUNDS::bld_base - 1;

		(*ppSd)->m_cx = ptrMmio->ReadShort ();
		(*ppSd)->m_cy = ptrMmio->ReadShort ();

		// CStructureData stuff
		(*ppSd)->m_iBldgCat = ptrMmio->ReadShort ();
		(*ppSd)->m_iTimeBuild = ptrMmio->ReadShort () * 24;
		(*ppSd)->m_iMoraleEffect = ptrMmio->ReadShort ();
		for (int iInd=0; iInd<3; iInd++)
			(*ppSd)->m_aiBuild [iInd] = ptrMmio->ReadShort ();
		for (iInd=3; iInd<CMaterialTypes::GetNumBuildTypes(); iInd++)
			(*ppSd)->m_aiBuild [iInd] = 0;
		(*ppSd)->m_iPower = ptrMmio->ReadShort ();
		(*ppSd)->m_fNoPower = (float) ptrMmio->ReadShort () / 100.0;

		// exit stuff
		(*ppSd)->m_hexExit.X () = ptrMmio->ReadShort ();
		(*ppSd)->m_hexExit.Y () = ptrMmio->ReadShort ();
		(*ppSd)->m_iExitDir = ptrMmio->ReadShort ();
		(*ppSd)->m_hexShip.X () = ptrMmio->ReadShort ();
		(*ppSd)->m_hexShip.Y () = ptrMmio->ReadShort ();
		(*ppSd)->m_iShipDir = ptrMmio->ReadShort ();
		(*ppSd)->m_bFlags = (CStructureData::BLDG_FLAGS) ptrMmio->ReadShort ();

		// CBuildUnion
		switch (iType)
		  {
			case CStructureData::UTmaterials : {
				CBuildMaterials * pBm = (*ppSd)->GetBldMaterials ();
				pBm->m_iTime = ptrMmio->ReadShort () * 24;
				for (iInd=0; iInd<CMaterialTypes::num_types; iInd++)
					pBm->m_aiInput [iInd] = ptrMmio->ReadShort ();
				for (iInd=0; iInd<CMaterialTypes::num_types; iInd++)
					pBm->m_aiOutput [iInd] = ptrMmio->ReadShort ();
				break; }

			case CStructureData::UTshipyard :
			case CStructureData::UTvehicle : {
				CBuildVehicle * pBv = (CBuildVehicle *) (*ppSd);	// cause ASSERT will fail
				ASSERT_STRICT ((pBv->m_iUnionType == CStructureData::UTvehicle) ||
													(pBv->m_iUnionType == CStructureData::UTshipyard));
				int iNum = ptrMmio->ReadShort ();
				while (iNum--)
					{
					CBuildUnit *pBu = new CBuildUnit ();
					pBu->m_iVehType = ptrMmio->ReadShort ();
					pBu->m_iTime = ptrMmio->ReadShort () * 24;
					for (iInd=0; iInd<CMaterialTypes::GetNumBuildTypes(); iInd++)
						pBu->m_aiInput [iInd] = ptrMmio->ReadShort ();
					pBv->m_aBldUnits.Add (pBu);
					}

				for (int iTyp=0; iTyp<pBv->GetSize (); iTyp++)
					{
					CBuildUnit const * pBldUnit = pBv->GetUnit (iTyp);
					for (int iOn=0; iOn<CMaterialTypes::GetNumBuildTypes(); iOn++)
						pBv->m_aiTotalInput[iOn] += pBldUnit->GetInput (iOn);
					}
				break; 
				}

			case CStructureData::UTrepair : {
				CBuildRepair * pBr = (*ppSd)->GetBldRepair ();
				int iNum = ptrMmio->ReadShort ();
				while (iNum--)
					{
					CBuildUnit *pBu = new CBuildUnit ();
					pBu->m_iVehType = ptrMmio->ReadShort ();
					pBu->m_iTime = ptrMmio->ReadShort () * 24;
					for (iInd=0; iInd<CMaterialTypes::GetNumBuildTypes(); iInd++)
						pBu->m_aiInput [iInd] = ptrMmio->ReadShort ();
					pBr->m_aRprUnits.Add (pBu);
					}

				for (int iTyp=0; iTyp<pBr->m_aRprUnits.GetSize (); iTyp++)
					{
					CBuildUnit const * pBldUnit = pBr->GetRepair (iTyp);
					for (int iOn=0; iOn<CMaterialTypes::GetNumBuildTypes(); iOn++)
						pBr->m_aiTotalRepair[iOn] += pBldUnit->GetInput (iOn);
					}
				break; }

			case CStructureData::UThousing : {
				CBuildHousing * pBh = (*ppSd)->GetBldHousing ();
				pBh->m_iCapacity = ptrMmio->ReadShort ();
				break; }

			case CStructureData::UTresearch : {
				CBuildResearch * pBr = (*ppSd)->GetBldResearch ();
				pBr->m_iRate = ptrMmio->ReadShort ();
				break; }

			case CStructureData::UTpower : {
				CBuildPower * pBp = (*ppSd)->GetBldPower ();
				pBp->m_iPower = ptrMmio->ReadShort ();
				pBp->m_iInput = ptrMmio->ReadShort ();
				pBp->m_iRate = ptrMmio->ReadShort ();
				break; }

			case CStructureData::UTmine : {
				CBuildMine * pBm = (*ppSd)->GetBldMine ();
				*ppSd = pBm;
				pBm->m_iTime = ptrMmio->ReadShort () * 24;
				pBm->m_iAmount = ptrMmio->ReadShort ();
				pBm->m_iOutput = ptrMmio->ReadShort ();
				break; }

			case CStructureData::UTfarm : {
				CBuildFarm * pBf = (*ppSd)->GetBldFarm ();
				*ppSd = pBf;
				pBf->m_iTime = ptrMmio->ReadShort () * 24;
				pBf->m_iOutput = ptrMmio->ReadShort ();
				pBf->m_iQuantity = ptrMmio->ReadShort ();
				break; }
		  }

		ptrMmio->AscendChunk ();

#ifdef _DEBUG
		for (int iBldgDir=0; iBldgDir<4; iBldgDir++)
			{
			CHexCoord hexExit;
			int iExitDir;
			CBuilding::DetermineExit (iBldgDir, iOn, hexExit, iExitDir, FALSE);
			if ((*ppSd)->HasShipExit ())
				CBuilding::DetermineExit (iBldgDir, iOn, hexExit, iExitDir, TRUE);
			}
#endif

		ASSERT_STRICT_VALID (*ppSd);
		}

	ptrMmio->AscendList ();

	m_bLoaded |= 0x01;

	theApp.BaseYield ();
}

void CStructure::InitSprites ()
{

	theApp.m_pCreateGame->GetDlgStatus()->SetPer (PER_STRUCTURE_INIT);

	// see if loaded
	if ( m_bLoaded & 0x02 )
		return;

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ( GetRifName(), "BLDG" );

	ptrMmio->DescendRiff ('B', 'L', 'D', 'G');

	Read(*ptrMmio, BUILDING_TIME, PER_STRUCTURE_INIT, PER_NUM_STRUCTURE_INIT, ptrthebltformat->GetBitsPerPixel() );

	CSprite	* psprite = GetSprite( CStructureData::farm, 0, TRUE );

	if ( psprite )
		psprite->SetTime( FARM_TIME );

	for ( int i=0; i<GetNumBuildings(); i++)
		{
		theApp.BaseYield ();

		CStructureData * pSd = _GetData (i);
		if ( GetSprite (i, 0, TRUE) != NULL )
			pSd->m_udFlags = (CUnitData::UNIT_DATA_FLAGS) (pSd->m_udFlags | CUnitData::FLhaveArt);
		}

#ifdef _CHEAT
	if (theApp.GetProfileInt ("Cheat", "ShowLoad", 0))
		theApp.m_pCreateGame->GetDlgStatus()->SetMsg ("Building bitmaps loaded");
#endif

	m_bLoaded |= 0x02;

	theApp.BaseYield ();

	ASSERT_VALID( this );

	#ifdef _DEBUG
	for ( i = 0; i < GetCount(); ++i )
		GetSpriteByIndex( i )->CheckValid();
	#endif
}

void CStructure::InitLang ()
{

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x04 )
		return;

	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO (NULL, "LANG");

	ptrMmio->DescendRiff ('L', 'A', 'N', 'G');

	// read in the structure types
	ptrMmio->DescendList ('T', 'Y', 'P', 'E');
	for (int iOn=0; iOn<theStructureType.GetNum (); iOn++)
		{
		ptrMmio->DescendChunk ('D', 'A', 'T', 'A');
		ptrMmio->ReadString (theStructureType.m_sTitle [iOn]);
		ptrMmio->AscendChunk ();
		}
	ptrMmio->AscendList ();

	theApp.BaseYield ();

	// read in the building descriptions
	ptrMmio->DescendList ('B', 'L', 'D', 'G');
	CStructureData * * ppSd = m_ppData;
	for (iOn=0; iOn<theStructures.GetNumBuildings (); iOn++, ppSd++)
		{
		ptrMmio->DescendChunk ('L', 'I', 'N', 'E');
		ptrMmio->ReadString ((*ppSd)->m_sDesc);
		ptrMmio->AscendChunk ();
		ptrMmio->DescendChunk ('D', 'E', 'S', 'C');
		ptrMmio->ReadString ((*ppSd)->m_sText);
		ptrMmio->AscendChunk ();
		}
	ptrMmio->AscendList ();

	// rotated bridges
	theApp.BaseYield ();

	MakeRotated( CStructureData::bridge_0, 0, CStructureData::bridge_1, 0, 1 );

	MakeRotated( CStructureData::bridge_end_0, 0, CStructureData::bridge_end_1, 0, 1 );
	MakeRotated( CStructureData::bridge_end_0, 0, CStructureData::bridge_end_2, 0, 2 );
	MakeRotated( CStructureData::bridge_end_0, 0, CStructureData::bridge_end_3, 0, 3 );

	ASSERT_STRICT_VALID (this);

	m_bLoaded |= 0x04;

	theApp.BaseYield ();
}

//---------------------------------------------------------------------------
// CStructure::MakeRotated
//---------------------------------------------------------------------------
void
CStructure::MakeRotated(
	int	iIDSrc,
	int	iIndexSrc,
	int	iIDDst,
	int	iIndexDst,
	int	iRotRight )
{
	CStructureSprite	* pstructuresprite = GetSprite( iIDSrc, iIndexSrc, TRUE );

	if ( !pstructuresprite )
		ThrowError( ERR_TLP_BAD_DATA );

	theApp.BaseYield ();

	Ptr< CSprite >	ptrsprite = new CStructureSprite( *pstructuresprite, iIDDst, iIndexDst, iRotRight );

	SetSprite( ptrsprite, iIDDst, iIndexDst );
}

/////////////////////////////////////////////////////////////////////////////
// CTransport - data on vehicles

void CTransport::InitData ()
{

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x01 )
		return;

	CTransportData::m_sAuto.LoadString ( IDS_TRUCK_AUTO );
	CTransportData::m_sRoute.LoadString ( IDS_TRUCK_ROUTE );
	CTransportData::m_sIdle.LoadString ( IDS_TRUCK_IDLE );
	CTransportData::m_sTravel.LoadString ( IDS_TRUCK_TRAVEL );

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ("units", "UNIT");

	ptrMmio->DescendRiff ('U', 'N', 'I', 'T');
	ptrMmio->DescendList ('V', 'E', 'H', 'L');

	ptrMmio->DescendChunk ('N', 'U', 'M', 'V');
	m_iNumTransports = ptrMmio->ReadShort ();
	ptrMmio->AscendChunk ();
	m_pData = new CTransportData [m_iNumTransports];

	CTransportData * pTd = m_pData;
	CVehicle::m_iMaxRange = 0;
	for (int iOn=0; iOn<theTransports.GetNumTransports (); iOn++, pTd++)
		{
		theApp.BaseYield ();

		ptrMmio->DescendChunk ('D', 'A', 'T', 'A');
		// CUnitData stuff
		pTd->m_iType = (CTransportData::TRANS_TYPE) iOn;
		pTd->ReadUnitData (*ptrMmio);

		if (pTd->m_iSoundIdle > 0)
			pTd->m_iSoundIdle += SOUNDS::veh_idle_base - 1;
		if (pTd->m_iSoundRun > 0)
			pTd->m_iSoundRun += SOUNDS::veh_running_base - 1;
		if (pTd->m_iSoundGo > 0)
			pTd->m_iSoundGo += SOUNDS::veh_go_base - 1;

		pTd->m_cx = pTd->m_cy = 1;

		// CTransport stuff
		pTd->m_iSetupFire = ptrMmio->ReadShort ();
		pTd->m_iPeopleCarry = ptrMmio->ReadShort ();
		pTd->m_iSpeed = ptrMmio->ReadShort ();
		pTd->m_cWheelType = ptrMmio->ReadShort ();
		pTd->m_cWaterDepth = ptrMmio->ReadShort ();
		pTd->m_transFlags = (CTransportData::TRANS_FLAGS) ptrMmio->ReadShort ();

		// get deepest water
		if (pTd->m_cWheelType == CWheelTypes::water)
			{
			pTd->m_cWaterDepth = __min ( 2, pTd->m_cWaterDepth );
			int iDraft = CHex::sea_level - pTd->m_cWaterDepth;
			if (iDraft < CTransportData::m_iMaxDraft)
				CTransportData::m_iMaxDraft = iDraft;
			}

		if (pTd->_GetSpottingRange () > CVehicle::m_iMaxRange)
			CVehicle::m_iMaxRange = pTd->_GetSpottingRange ();
		if (pTd->_GetRange () > CVehicle::m_iMaxRange)
			CVehicle::m_iMaxRange = pTd->_GetRange ();
		ASSERT_STRICT ((0 < CVehicle::m_iMaxRange) && 
						(CVehicle::m_iMaxRange + CVehicle::m_iMaxRange / 2 <= MAX_SPOTTING));

		ptrMmio->AscendChunk ();
		}

	ptrMmio->AscendList ();

	// create the spotting range table
	CVehicle::m_iMaxRange += CVehicle::m_iMaxRange / 2 + 2;

	ASSERT_STRICT ((0 < CVehicle::m_iMaxRange) && (CVehicle::m_iMaxRange <= MAX_SPOTTING));
	ASSERT_STRICT (CVehicle::m_apiWid == NULL);
	CVehicle::m_apiWid = new int * [CVehicle::m_iMaxRange + 1];
	memset (CVehicle::m_apiWid, 0, sizeof (int *) * CVehicle::m_iMaxRange);

	for (int iRange=0; iRange<CVehicle::m_iMaxRange; iRange++)
		{
		int *piOn = CVehicle::m_apiWid[iRange] = new int [iRange * 2 + 2];
		int *piEnd = piOn + iRange * 2;
		*(piEnd + 1) = 0;	// needed by NextVisible
		int iRsqr = iRange * iRange;
		for (int iWid=iRange; iWid>=0; iWid--, piOn++, piEnd--)
			*piOn = *piEnd = sqrt (( double )( iRsqr - (iWid * iWid)));
		}

	m_bLoaded |= 0x01;

	theApp.BaseYield ();
}

void CTransport::InitSprites ()
{

	theApp.m_pCreateGame->GetDlgStatus()->SetPer (PER_TRANSPORT_INIT);

	// see if loaded
	if ( m_bLoaded & 0x02 )
		return;

	// read in the RIF data
	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ( GetRifName(), "VEHL" );

	ptrMmio->DescendRiff ('V', 'E', 'H', 'L');

	Read(*ptrMmio, VEHICLE_TIME, PER_TRANSPORT_INIT, PER_NUM_TRANSPORT_INIT, ptrthebltformat->GetBitsPerPixel() );

	theApp.BaseYield ();

#ifdef _CHEAT
	if (theApp.GetProfileInt ("Cheat", "ShowLoad", 0))
		theApp.m_pCreateGame->GetDlgStatus()->SetMsg ("Vehicle bitmaps loaded");
#endif

	int	i;

	#ifdef _DEBUG

	// Check flag hotspots

	for ( i = 0; i < GetCount(); ++i )
	{
		CVehicleSprite *psprite = GetSpriteByIndex( i );

		// ASSERT( 1 == psprite->GetNumHotSpots( CHotSpotKey::FLAG_HOTSPOT )); GGTODO: uncomment when art fixed

		for ( int iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
			for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
				for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
				{
					CSpriteView * pspriteview = psprite->GetView( iDir, iTilt, iDamage );

					CHotSpotKey	hotspotkey( CHotSpotKey::FLAG_HOTSPOT, 0 );
					CHotSpot const * photspot = pspriteview->GetHotSpot( hotspotkey );

					// ASSERT( photspot ); GGTODO: uncomment when art fixed
				}
	}

	theApp.BaseYield ();

	// Check smoke/flame hotspots

	for ( i = 0; i < GetCount(); ++i )
	{
		CVehicleSprite *psprite = GetSpriteByIndex( i );

		for ( int iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
			for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
				for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
				{
					CSpriteView * pspriteview = psprite->GetView( iDir, iTilt, iDamage );

					for ( int iSpot = 0; iSpot < 3; ++iSpot )
					{
						CHotSpotKey	hotspotkey( CHotSpotKey::SMOKE_FLAME_HOTSPOT, iSpot );
						CHotSpot const * photspot = pspriteview->GetHotSpot( hotspotkey );

						// ASSERT( photspot ); GGTODO: uncomment when art fixed
					}
				}
	}

	#endif

	for (i=0; i<GetNumTransports(); i++)
		{
		theApp.BaseYield ();

		CTransportData * pTd = _GetData (i);
		if ( CSpriteStore< CVehicleSprite >::GetSprite (i, 0, TRUE) != NULL )
			pTd->m_udFlags = (CUnitData::UNIT_DATA_FLAGS) (pTd->m_udFlags | CUnitData::FLhaveArt);

		if ( pTd->GetUnitFlags () & ( CUnitData::FLflash1 | CUnitData::FLflash2) )
			{
			// set muzzle flash to fast
			CVehicleSprite *psprite = GetSprite( i );
// GG			CVehicleSprite *psprite = GetSpriteByIndex( i );
			for ( int v = 0; v < psprite->GetNumViews(); ++v )
				{
				CSpriteView * pspriteview = psprite->GetSpriteView( v );
				if ( pTd->GetUnitFlags () & CUnitData::FLflash1 )
					for ( int k = 0; k < pspriteview->AnimCount (CSpriteView::ANIM_FRONT_1); ++k )
						{
						CSpriteDIB	* pspritedib = ( CSpriteDIB * )pspriteview->GetAnim (CSpriteView::ANIM_FRONT_1, k );
						pspritedib->Time( 2 );
						}
				if ( pTd->GetUnitFlags () & CUnitData::FLflash2 )
					for ( int k = 0; k < pspriteview->AnimCount (CSpriteView::ANIM_FRONT_2); ++k )
						{
						CSpriteDIB	* pspritedib = ( CSpriteDIB * )pspriteview->GetAnim (CSpriteView::ANIM_FRONT_2, k );
						pspritedib->Time( 2 );
						}
				}
			}
		}

#ifdef _CHEAT
	if (theApp.GetProfileInt ("Cheat", "ShowLoad", 0))
		theApp.m_pCreateGame->GetDlgStatus()->SetMsg ("Vehicle hotspots loaded");
#endif

	ASSERT_VALID( this );

	#ifdef _DEBUG
	for ( i = 0; i < GetCount(); ++i )
		GetSpriteByIndex( i )->CheckValid();
	#endif

	m_bLoaded |= 0x02;

	theApp.BaseYield ();
}

void CTransport::InitLang ()
{

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x04 )
		return;

	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO (NULL, "LANG");

	ptrMmio->DescendRiff ('L', 'A', 'N', 'G');
	ptrMmio->DescendList ('V', 'E', 'H', 'L');

	CTransportData * pTd = m_pData;
	for (int iOn=0; iOn<theTransports.GetNumTransports (); iOn++, pTd++)
		{
		ptrMmio->DescendChunk ('L', 'I', 'N', 'E');
		ptrMmio->ReadString (pTd->m_sDesc);
		ptrMmio->AscendChunk ();
		ptrMmio->DescendChunk ('D', 'E', 'S', 'C');
		ptrMmio->ReadString (pTd->m_sText);
		ptrMmio->AscendChunk ();
		}
	ptrMmio->AscendList ();

	ASSERT_STRICT_VALID (this);

	m_bLoaded |= 0x04;

	theApp.BaseYield ();
}

void CTransport::Close ()
{

	ASSERT_STRICT_VALID (this);
	CSpriteStore< CVehicleSprite >::Close ();
	delete [] m_pData;
	m_pData = NULL;

	if (CVehicle::m_apiWid != NULL)
		{
		for (int iRange=0; iRange<CVehicle::m_iMaxRange; iRange++)
			delete [] CVehicle::m_apiWid [iRange];
		delete [] CVehicle::m_apiWid;
		CVehicle::m_apiWid = NULL;
		}
}

/////////////////////////////////////////////////////////////////////////////
// CEffect - Sprites with no direction or damage

//-------------------------------------------------------------------------
// CEffect::InitData
//-------------------------------------------------------------------------
void CEffect::InitData ()
{
}

//-------------------------------------------------------------------------
// CEffect::InitSprites
//-------------------------------------------------------------------------
void CEffect::InitSprites()
{
	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x02 )
		return;

	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO ( GetRifName(), "EFFX" );

	ptrMmio->DescendRiff ('E', 'F', 'F', 'X');

	m_nTrees = 0;

	Read(*ptrMmio,
		   EFFECT_TIME,
			PER_EFFECTS_INIT,
			PER_NUM_EFFECTS_INIT,
			ptrthebltformat->GetBitsPerPixel(),
			CSprite::EFFECT );

	// Store tree tiles

	m_nTrees = GetCount( tree );

	ASSERT( 0 < m_nTrees );

	if ( !( 0 < m_nTrees ))
		ThrowError( ERR_TLP_BAD_DATA );

	m_ptrees = new CTree [ m_nTrees ];

	for ( int i = 0; i < m_nTrees; ++i )
		{
		theApp.BaseYield ();
		m_ptrees[i].m_psprite = GetSprite( tree, i );
		}

	#ifdef _DEBUG

	int	iFirstZoom = theApp.GetZoomData()->GetFirstZoom();

	// Check flag anchor

	theApp.BaseYield ();

	for ( i = 0; i < GetCount( flag ); ++i )
	{
		CEffectSprite	* pspriteFlag = GetSprite( flag, i );

		ASSERT( pspriteFlag );

		CSpriteView *	pspriteview = pspriteFlag->GetView();
						
		CPoint	ptAnchor = pspriteview->GetAnchorPoint( iFirstZoom );

		// ASSERT( CPoint( 0, 0 ) != ptAnchor ); GGTODO: uncomment when hotspot art available
	}

	// Check smoke anchor

	theApp.BaseYield ();

	for ( i = 0; i < GetCount( smoke ); ++i )
	{
		CEffectSprite	* pspriteSmoke = GetSprite( smoke, i );

		ASSERT( pspriteSmoke );

		CSpriteView *	pspriteview = pspriteSmoke->GetView();
			  
		CPoint	ptAnchor = pspriteview->GetAnchorPoint( iFirstZoom );

		// ASSERT( CPoint( 0, 0 ) != ptAnchor ); GGTODO: uncomment when hotspot art available
	}

	// Check flame anchor

	theApp.BaseYield ();

	for ( i = 0; i < GetCount( fire ); ++i )
	{
		CEffectSprite	* pspriteFlame = GetSprite( fire, i );

		ASSERT( pspriteFlame );

		CSpriteView *	pspriteview = pspriteFlame->GetView();
		 
		CPoint	ptAnchor = pspriteview->GetAnchorPoint( iFirstZoom );

		// ASSERT( CPoint( 0, 0 ) != ptAnchor ); GGTODO: uncomment when hotspot art available
	}

	#endif

	m_bLoaded |= 0x02;

	theApp.BaseYield ();

	#ifdef _DEBUG
	for ( i = 0; i < GetCount(); ++i )
		GetSpriteByIndex( i )->CheckValid();
	#endif

	ASSERT_VALID( this );
}

//-------------------------------------------------------------------------
// CEffect::Close
//-------------------------------------------------------------------------
void
CEffect::Close()
{
	ASSERT_VALID (this);

	CSpriteStore< CEffectSprite >::Close ();

	delete [] m_ptrees;

	m_ptrees = NULL;
}

//-------------------------------------------------------------------------
// CTurrets::InitData
//-------------------------------------------------------------------------
void CTurrets::InitData ()
{
}

//-------------------------------------------------------------------------
// CTurrets::InitSprites
//-------------------------------------------------------------------------
void CTurrets::InitSprites()
{
//	pDlg->SetPer( PER_TURRET_INIT );	FIXIT?

	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x02 )
		return;

	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO( GetRifName(), "TURR" );

	ptrMmio->DescendRiff( 'T', 'U', 'R', 'R' );

	Read(*ptrMmio,
		   VEHICLE_TIME,
			PER_TURRETS_INIT,
			PER_NUM_TURRETS_INIT,
			ptrthebltformat->GetBitsPerPixel() );

	#ifdef _DEBUG
	{
		// Check the anchor points

		theApp.BaseYield ();

		int	iFirstZoom = theApp.GetZoomData()->GetFirstZoom();

		for ( int i = 0; i < GetCount(); ++i )
		{
			CVehicleSprite	* pspriteTurret = GetSpriteByIndex( i );

			for ( int iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
				for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
					for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
					{
						CSpriteView * pspriteview = pspriteTurret->GetView( iDir, iTilt, iDamage );

						CPoint	ptAnchor = pspriteview->GetAnchorPoint( iFirstZoom );

						ASSERT( CPoint( 0, 0 ) != ptAnchor );
					}
		}

		// Check muzzle hotspots

		theApp.BaseYield ();

		for ( i = 0; i < GetCount(); ++i )
		{
			CVehicleSprite *pspriteTurret = GetSpriteByIndex( i );
		
			// ASSERT( 1 == pspriteTurret->GetNumHotSpots( CHotSpotKey::MUZZLE_HOTSPOT )); GGTODO: uncomment when art is fixed

			for ( int iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
				for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
					for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
					{
						CSpriteView * pspriteview = pspriteTurret->GetView( iDir, iTilt, iDamage );

						CHotSpotKey	hotspotkey( CHotSpotKey::MUZZLE_HOTSPOT, 0 );
						CHotSpot	const * photspot = pspriteview->GetHotSpot( hotspotkey );

						// ASSERT( photspot ); GGTODO: uncomment when art fixed
					}
		}
	}
	#endif

	ASSERT_VALID( this );

	#ifdef _DEBUG
	for ( int i = 0; i < GetCount(); ++i )
		GetSpriteByIndex( i )->CheckValid();
	#endif

	m_bLoaded |= 0x02;

	theApp.BaseYield ();
}

//-------------------------------------------------------------------------
// CMuzzleFlashes::InitData
//-------------------------------------------------------------------------
void CMuzzleFlashes::InitData ()
{
}

//-------------------------------------------------------------------------
// CMuzzleFlash::InitSprites
//-------------------------------------------------------------------------
void CMuzzleFlashes::InitSprites()
{
	theApp.BaseYield ();

	// see if loaded
	if ( m_bLoaded & 0x02 )
		return;

	Ptr< CMmio > ptrMmio = theDataFile.OpenAsMMIO( GetRifName(), "FLAS" );

	ptrMmio->DescendRiff( 'F', 'L', 'A', 'S' );

	Read(*ptrMmio,
		   EFFECT_TIME,
			PER_MUZZLE_FLASHES_INIT,
			PER_NUM_MUZZLE_FLASHES_INIT,
			ptrthebltformat->GetBitsPerPixel() );

	#ifdef _DEBUG
	{
		theApp.BaseYield ();
	
		// Check the anchor points

		int	iFirstZoom = theApp.GetZoomData()->GetFirstZoom();

		for ( int i = 0; i < GetCount(); ++i )
		{
			CVehicleSprite	* pspriteMuzzleFlash = GetSpriteByIndex( i );

			for ( int iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
				for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
					for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
					{
						CSpriteView * pspriteview = pspriteMuzzleFlash->GetView( iDir, iTilt, iDamage );

						CPoint	ptAnchor = pspriteview->GetAnchorPoint( iFirstZoom );

						ASSERT( CPoint( 0, 0 ) != ptAnchor );
					}
		}
	}
	#endif

	m_bLoaded |= 0x02;

	theApp.BaseYield ();

	ASSERT_VALID( this );

	#ifdef _DEBUG
	for ( int i = 0; i < GetCount(); ++i )
		GetSpriteByIndex( i )->CheckValid();
	#endif
}

//-------------------------------------------------------------------------
// CViewCoord::CViewCoord
//-------------------------------------------------------------------------
CViewCoord::CViewCoord(
	CPoint const & pt )
{
	int	iFirstZoom = theApp.GetZoomData()->GetFirstZoom();

	m_apt[ iFirstZoom ] = pt;

	for ( int i = iFirstZoom + 1; i < NUM_ZOOM_LEVELS; ++i )
	{
		m_apt[ i ].x = m_apt[ iFirstZoom ].x >> i;
		m_apt[ i ].y = m_apt[ iFirstZoom ].y >> i;
	}
}

//-------------------------------------------------------------------------
// CSpriteView::Init
//-------------------------------------------------------------------------
void
CSpriteView::Init(
	CSprite			 * psprite,
	CSpriteDIBParms * pspritedibparms )
{
	m_psprite            = psprite;
	m_photspots 	      = ( CHotSpot   * )( m_anAnim    			  + ANIM_COUNT    );
	m_pspritedibBase     = ( CSpriteDIB * )( m_photspots 			  + m_nHotSpots   );
	m_pspritedibOverlay  = ( CSpriteDIB * )( m_pspritedibBase 	  + m_nBase       );
	m_apspritedibAnim[0] = ( CSpriteDIB * )( m_pspritedibOverlay  + m_nOverlay    );
	m_apspritedibAnim[1] = ( CSpriteDIB * )( m_apspritedibAnim[0] + m_anAnim[ 0 ] );
	m_apspritedibAnim[2] = ( CSpriteDIB * )( m_apspritedibAnim[1] + m_anAnim[ 1 ] );
	m_apspritedibAnim[3] = ( CSpriteDIB * )( m_apspritedibAnim[2] + m_anAnim[ 2 ] );

	pspritedibparms->m_pspriteview = this;

	int	i, j;

	theApp.BaseYield ();
	if ( m_nBase )
		for ( i = 0; i < m_nBase; ++i )
			m_pspritedibBase[i].Init( *pspritedibparms );

	theApp.BaseYield ();
	if ( m_nOverlay )
		for ( i = 0; i < m_nOverlay; ++i )
			m_pspritedibOverlay[i].Init( *pspritedibparms );

	theApp.BaseYield ();
	for ( j = 0; j < ANIM_COUNT; ++j )
		if ( m_anAnim[j] )
			for ( i = 0; i < m_anAnim[j]; ++i )
				m_apspritedibAnim[j][i].Init( *pspritedibparms );

	#ifdef _DEBUG
	CheckValid();
	#endif
}

//-------------------------------------------------------------------------
// CSpriteDIB::Init
//-------------------------------------------------------------------------
void
CSpriteDIB::Init(
	CSpriteDIBParms const & spritedibparms )
{
	m_uTime 			  = spritedibparms.m_uTime;
	m_iType          = spritedibparms.m_iType;
	m_iBytesPerPixel = ( spritedibparms.m_iBitsPerPixel + 7 ) >> 3;
	m_pspriteview    = spritedibparms.m_pspriteview;

	#ifdef _DEBUG
	CheckValid();
	#endif
}

//-------------------------------------------------------------------------
// CSpriteView::CalcRect
//-------------------------------------------------------------------------
void
CSpriteView::CalcRect()
{
	int	iZoom, j, k;

	for ( iZoom = theApp.GetZoomData()->GetFirstZoom(); iZoom < NUM_ZOOM_LEVELS; ++iZoom )
	{
		m_arect[ iZoom ] = CRect( 0, 0, 0, 0 );

		for ( j = 0; j < m_nBase; ++j )
			m_arect[ iZoom ] |= m_pspritedibBase[ j ].Rect( iZoom );

		for ( j = 0; j < m_nOverlay; ++j )
			m_arect[ iZoom ] |= m_pspritedibOverlay[ j ].Rect( iZoom );

		for ( k = 0; k < ANIM_COUNT; ++k )
			for ( j = 0; j < m_anAnim[ k ]; ++j )
				m_arect[ iZoom ] |= m_apspritedibAnim[ k ][ j ].Rect( iZoom );
	}
}

//-------------------------------------------------------------------------
// CSpriteView::SetOverlayBase
//-------------------------------------------------------------------------
void
CSpriteView::SetOverlayBase(
	const CSpriteView *pspriteviewNoDamage )
{
	#ifdef _DEBUG
	pspriteviewNoDamage->CheckValid();
	#endif

	if ( BaseCount() )
		return;	// Already got 'em

	#ifdef _DEBUG
	ASSERT( 0 <  m_nOverlay );
	ASSERT( pspriteviewNoDamage->BaseCount() );
	#endif

	CopyBase( pspriteviewNoDamage );
}

//-------------------------------------------------------------------------
// CSpriteView::SetAnimBase
//-------------------------------------------------------------------------
void
CSpriteView::SetAnimBase(
	const CSpriteView *pspriteviewNoDamage )
{
	#ifdef _DEBUG
	pspriteviewNoDamage->CheckValid();
	#endif

	#ifdef _DEBUG
	CheckValid();
	#endif

	for ( int k = 0; k < ANIM_COUNT; ++k )
		if ( 0 == m_anAnim[ k ] &&
			  0 != pspriteviewNoDamage->AnimCount( CSpriteView::ANIM_TYPE( k )))
			CopyAnims( pspriteviewNoDamage, CSpriteView::ANIM_TYPE( k ));

	#ifdef _DEBUG
	CheckValid();
	#endif
}

//-------------------------------------------------------------------------
// CSpriteView::CopyBase
//-------------------------------------------------------------------------
void
CSpriteView::CopyBase(
	const CSpriteView *pspriteviewSrc )
{
	m_nBase			  = pspriteviewSrc->BaseCount();
	m_pspritedibBase = pspriteviewSrc->m_pspritedibBase;
}

//-------------------------------------------------------------------------
// CSpriteView::CopyAnims
//-------------------------------------------------------------------------
void
CSpriteView::CopyAnims(
	const CSpriteView * pspriteviewSrc,
	ANIM_TYPE 			  eAnim )
{
	m_anAnim[ eAnim ]			   = pspriteviewSrc->AnimCount( eAnim );
	m_apspritedibAnim[ eAnim ] = pspriteviewSrc->m_apspritedibAnim[ eAnim ];
}

//---------------------------------------------------------------------------
// CSpriteParms::CSpriteParms
//---------------------------------------------------------------------------
CSpriteParms::CSpriteParms(
	CMmio  * pmmio,
	unsigned	uTime,
	int 	   iTypeOverride,
	int 	   iBitsPerPixel )
	:
		m_uTime				 ( uTime ),
		m_iBitsPerPixel    ( iBitsPerPixel ),
		m_pspritecollection( NULL )
{
	ASSERT( -1 == iTypeOverride || ( 0 < iTypeOverride && iTypeOverride <= MAX_SPRITE_TYPES ));

	if ( !( -1 == iTypeOverride || ( 0 < iTypeOverride && iTypeOverride <= MAX_SPRITE_TYPES )))
		ThrowError( ERR_TLP_BAD_DATA );

	pmmio->DescendChunk( 'D', 'A', 'T', 'A' );

	m_iID = pmmio->ReadShort();

	ASSERT( 0 <= m_iID && m_iID < 5000 );

	if ( !( 0 <= m_iID && m_iID < 5000 ))
		ThrowError( ERR_TLP_BAD_DATA );

	int	lLenTotal = pmmio->ReadLong();

	m_iType = pmmio->ReadLong();

	if ( -1 != iTypeOverride )
		m_iType = iTypeOverride;

	m_bDummy = DUMMY_SPRITE == lLenTotal;

	if ( m_bDummy )
	{
	 	pmmio->AscendChunk ();
		return;
	}

	long	lHdrLen = pmmio->ReadLong();

//BUGBUG	m_ptrspritehdr = ( CSpriteHdr * )( new BYTE [ lHdrLen ] );
	m_ptrspritehdr = ( CSpriteHdr * ) MemAllocPtr ( _memPoolCompSprites, lHdrLen, 0 );
	if ( m_ptrspritehdr.Value () == NULL )
		{
		AfxMessageBox ( IDS_NO_MEMORY, MB_OK );
		ThrowError ( ERR_OUT_OF_MEMORY );
		}

	pmmio->Read( m_ptrspritehdr.Value(), lHdrLen );

	// Load the pixels

	pmmio->ReadLong();	// Length of 4-zooms of pixel data

	int	iFirstZoom = theApp.GetZoomData()->GetFirstZoom();
	int	iNumZooms  = NUM_ZOOM_LEVELS - iFirstZoom;
	long	lLength    = 0L;

	for ( int i = 0; i < iNumZooms; i++ )
		lLength += m_ptrspritehdr->m_blockinfoZoom[ NUM_ZOOM_LEVELS - 1 - i ].m_lLength;

	// theApp.BaseYield ();

//BUGBUG	m_ptrbyCompressedSuperviews = new BYTE [ lLength ];
	m_ptrbyCompressedSuperviews = ( BYTE * )MemAllocPtr ( _memPoolCompSprites, lLength, 0 );
	if ( m_ptrbyCompressedSuperviews.Value () == NULL )
		{
		AfxMessageBox ( IDS_NO_MEMORY, MB_OK );
		ThrowError ( ERR_OUT_OF_MEMORY );
		}

	//	theApp.BaseYield ();

	pmmio->Read( m_ptrbyCompressedSuperviews.Value(), lLength );
 	pmmio->AscendChunk ();

	//	theApp.BaseYield ();
}

//---------------------------------------------------------------------------
// CSprite::SetTime - Set hold time for all ambients of all views
//---------------------------------------------------------------------------
void
CSprite::SetTime(
	unsigned uTime )
{
	for ( int i = 0; i < GetNumViews(); ++i )
	{
		CSpriteView * pspriteview = GetSpriteView( i );

		for ( int j = 0; j < CSpriteView::ANIM_COUNT; ++j )
			for ( int k = 0; k < pspriteview->AnimCount(( CSpriteView::ANIM_TYPE )j ); ++k )
			{
				CSpriteDIB	* pspritedib = ( CSpriteDIB * )pspriteview->GetAnim(( CSpriteView::ANIM_TYPE )j, k );

				pspritedib->Time( uTime );
			}
	}
}

//---------------------------------------------------------------------------
// CSprite::VirtualConstruct
//---------------------------------------------------------------------------
Ptr< CSprite >
CSprite::VirtualConstruct(
	CSpriteParms const & spriteparms )
{
	Ptr< CSprite >	ptrsprite;

	switch ( spriteparms.m_iType )
	{
		case TERRAIN:		ptrsprite = new CTerrainSprite  ( spriteparms ); break;
		case EFFECT:		ptrsprite = new CEffectSprite   ( spriteparms ); break;
		case STRUCTURE:	ptrsprite = new CStructureSprite( spriteparms ); break; 
		case VEHICLE:		ptrsprite = new CVehicleSprite  ( spriteparms ); break;

		default:	ASSERT( 0 );
					ThrowError ( ERR_TLP_BAD_DATA );

	}

	#ifdef _DEBUG
	ptrsprite->CheckValid();
	#endif

	return ptrsprite;
}

//---------------------------------------------------------------------------
// CSprite::CSprite
//---------------------------------------------------------------------------
CSprite::CSprite(
	CSpriteParms const & spriteparms )
	:
		m_piViewOffsets	 ( NULL ),
		m_piViewIndices	 ( NULL ),
		m_iBitsPerPixel	 ( spriteparms.m_iBitsPerPixel ),
		m_iID					 ( spriteparms.m_iID ),
		m_pspritecollection( spriteparms.m_pspritecollection ),
		m_iIndex				 ( 0 )
{
	theApp.BaseYield ();

	m_iBytesPerPixel = ( m_iBitsPerPixel + 7 ) >> 3;
	m_iType 			  = spriteparms.m_iType;

	memset( m_anHotSpots, 0, sizeof( m_anHotSpots ));

	m_ptrspritehdr = spriteparms.m_ptrspritehdr;

	if ( ! m_ptrspritehdr.Value() )
		return;

	int	nViews = GetNumViews();

	m_psuperviewinfo = ( CSuperviewInfo * )m_ptrspritehdr->m_abyTheRest;
	m_piViewOffsets  = ( int * )( m_psuperviewinfo + GetNumSuperviews() );
	m_piViewIndices  = m_piViewOffsets + GetNumViews();

	CSpriteDIBParms	spritedibparms( spriteparms.m_uTime, m_iType, m_iBitsPerPixel );

	for ( int i = 0; i < GetNumViews(); ++i )
	{
		CSpriteView	* pspriteview = GetSpriteView( i, FALSE );

		pspriteview->Init( this, &spritedibparms );

		// Store max hotspot index for each hotspot type

		int	nHotSpots = pspriteview->m_nHotSpots;

		for ( int j = 0; j < nHotSpots; ++j )
			AddHotSpotKey( pspriteview->m_photspots[ j ].m_key );
	}

   SimplePtr< BYTE > ptrbyCompressedSuperviews = spriteparms.m_ptrbyCompressedSuperviews;

   // Get buffer length for decompressed sprite pixels

   int   iDecompressedTotalLength = 0;
   int   iSrcBytesPerPixel = 1 == m_iBytesPerPixel ? 1 : 3;
   int   iZoom;
   int   iSuperview;
	int	iFirstZoom = theApp.GetZoomData()->GetFirstZoom();

   for ( iZoom = iFirstZoom; iZoom < NUM_ZOOM_LEVELS; iZoom++ )
      for ( iSuperview = 0; iSuperview < GetNumSuperviews(); iSuperview++ )
         iDecompressedTotalLength += m_psuperviewinfo[ iSuperview ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[ iSrcBytesPerPixel - 1 ];

   // Decompress pixels into buffer

//   SimplePtr< BYTE > ptrbyDecompressedSuperviews = new BYTE [ iDecompressedTotalLength ];
		SimplePtr< BYTE > ptrbyDecompressedSuperviews;
		if ( 1 == m_iBytesPerPixel || 3 == m_iBytesPerPixel )
			ptrbyDecompressedSuperviews = ( BYTE * ) MemAllocPtr ( _memPoolSprites, iDecompressedTotalLength, 0 );
		else
			ptrbyDecompressedSuperviews = ( BYTE * ) MemAllocPtr ( _memPoolCompSprites, iDecompressedTotalLength, 0 );

   BYTE            * pbyCompressedSuperview      = ptrbyCompressedSuperviews.Value();
   BYTE            * pbyDecompressedSuperview    = ptrbyDecompressedSuperviews.Value();

   for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= iFirstZoom; iZoom-- )
      for ( iSuperview = 0; iSuperview < GetNumSuperviews(); iSuperview++ )
      {
         int	iCompressedSuperviewLength = m_psuperviewinfo[ iSuperview ].m_alayoutinfo[ iZoom ].m_blockinfoCompressed.m_lLength;
			int	iDecompressedSuperviewLength;

			if ( 0 == iCompressedSuperviewLength )
				continue;

				 theApp.BaseYield ();

         CoDec::Decompress( pbyCompressedSuperview, iCompressedSuperviewLength, pbyDecompressedSuperview, iDecompressedSuperviewLength );

         pbyCompressedSuperview   += iCompressedSuperviewLength;
         pbyDecompressedSuperview += iDecompressedSuperviewLength;
      }

   // Color convert pixel data, if necessary

   if ( 1 == m_iBytesPerPixel || 3 == m_iBytesPerPixel )
      m_ptrbyDecompressedSuperviews = ptrbyDecompressedSuperviews;
   else
   {
      int   iDecompressedLength = 0;

      for ( iZoom = iFirstZoom; iZoom < NUM_ZOOM_LEVELS; iZoom++ )
         for ( iSuperview = 0; iSuperview < GetNumSuperviews(); iSuperview++ )
            iDecompressedLength += m_psuperviewinfo[ iSuperview ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[ m_iBytesPerPixel - 1 ];

//BUGBUG      m_ptrbyDecompressedSuperviews = new BYTE [ iDecompressedLength ];
			m_ptrbyDecompressedSuperviews = ( BYTE * ) MemAllocPtr ( _memPoolSprites, iDecompressedLength, 0 );

      BYTE  * pbyDst = m_ptrbyDecompressedSuperviews.Value();
      BYTE  * pbySrc = ptrbyDecompressedSuperviews.Value();

      for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= iFirstZoom; iZoom-- )
         for ( iSuperview = 0; iSuperview < GetNumSuperviews(); iSuperview++ )
         {
            int   iLenSrc = m_psuperviewinfo[ iSuperview ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[ iSrcBytesPerPixel - 1 ];
            int   iLenDst = m_psuperviewinfo[ iSuperview ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[ m_iBytesPerPixel  - 1 ];

				if ( 0 == iLenSrc )
					continue;

						theApp.BaseYield ();

            ColorConvert( pbyDst, pbySrc, iLenDst, iLenSrc, iZoom, iSuperview );

            pbyDst += iLenDst;
            pbySrc += iLenSrc;
         }
   }

   // Assign superview pointers

   pbyDecompressedSuperview = m_ptrbyDecompressedSuperviews.Value();

   for ( iZoom = NUM_ZOOM_LEVELS - 1; iZoom >= iFirstZoom; iZoom-- )
      for ( iSuperview = 0; iSuperview < GetNumSuperviews(); iSuperview++ )
      {
         int   iDecompressedSuperviewLength = m_psuperviewinfo[ iSuperview ].m_alayoutinfo[ iZoom ].m_aiDecompressedLength[ m_iBytesPerPixel - 1 ];

         m_apbyDecompressedSuperviews[ iZoom ][ iSuperview ] = pbyDecompressedSuperview;

         pbyDecompressedSuperview += iDecompressedSuperviewLength;
      }

	theApp.BaseYield ();

	#ifdef _DEBUG
	CheckValid();
	#endif
}

//-------------------------------------------------------------------------
// CSprite::PostConstruct
//-------------------------------------------------------------------------
void
CSprite::PostConstruct()
{
	for ( int i = 0; i < GetNumViews(); ++i )
	{
		CSpriteView	* pspriteview = GetSpriteView( i );

		pspriteview->CalcRect();
	}
}

//-------------------------------------------------------------------------
// CSprite::AddHotSpotKey
//-------------------------------------------------------------------------
void
CSprite::AddHotSpotKey(
	CHotSpotKey const & hotspotkey )
{
	int	n = hotspotkey.m_iIndex + 1;

	if ( n > m_anHotSpots[ hotspotkey.m_eType ] )
		m_anHotSpots[ hotspotkey.m_eType ] = n;
}

//---------------------------------------------------------------------------
// CTerrainSprite::CTerrainSprite
//---------------------------------------------------------------------------
CTerrainSprite::CTerrainSprite(
	CSpriteParms const & spriteparms )
	:
		CSprite( spriteparms )
{

	theApp.BaseYield ();

	memset( m_aapspriteview, 0, sizeof( m_aapspriteview ));

	if ( spriteparms.m_bDummy )
		return;

	int			iIndex;
	int			iDir;
	int			iDamage;
	int const * piIndex = GetViewIndices();

	for ( iDir = 0; iDir < NUM_TERRAIN_DIRECTIONS; ++iDir )
		for ( iDamage = 0; iDamage < NUM_TERRAIN_DAMAGE; ++iDamage )
		{
			iIndex = *piIndex++;

			ASSERT( -1 <= iIndex && iIndex < GetNumViews() );
		
			if ( !( -1 <= iIndex && iIndex < GetNumViews() ))
				ThrowError ( ERR_TLP_BAD_DATA );

			if ( -1 != iIndex )
				m_aapspriteview[ iDir ][ iDamage ] = GetSpriteView( iIndex );
		}

	for ( iDir = 0; iDir < NUM_TERRAIN_DIRECTIONS; ++iDir )
		for ( iDamage = 0; iDamage < NUM_TERRAIN_DAMAGE; ++iDamage )
		{
			if ( !m_aapspriteview[ iDir ][ iDamage ] )
			{
				if ( 0 < iDamage )
				{
					ASSERT( AfxIsValidAddress( m_aapspriteview[ iDir ][ iDamage - 1 ], sizeof( CSpriteView )));

					m_aapspriteview[ iDir ][ iDamage ] = m_aapspriteview[ iDir ][ iDamage - 1 ];
				}
				else if ( 0 < iDir )
				{
					ASSERT( AfxIsValidAddress( m_aapspriteview[ iDir - 1 ][ iDamage ], sizeof( CSpriteView )));

					m_aapspriteview[ iDir ][ iDamage ] = m_aapspriteview[ iDir - 1 ][ iDamage ];
				}
				else
				{
					ASSERT( 0 );
					ThrowError ( ERR_TLP_BAD_DATA );
				}
			}

			// No damage overlays for terrain, for now at least
		}

	PostConstruct();

	#ifdef _DEBUG
	CheckValid();
	#endif;
}
	
//---------------------------------------------------------------------------
// CTerrainSprite::CTerrainSprite
//---------------------------------------------------------------------------
CTerrainSprite::CTerrainSprite(
	CTerrainSprite const & terrainsprite,
	int	 					  iID,
	int	 					  iIndex,
	int 						  iRotRight )
	:
		CSprite( terrainsprite )
{
	ASSERT( 0 <= iRotRight && iRotRight < NUM_TERRAIN_DIRECTIONS );

	SetID   ( iID    );
	SetIndex( iIndex );

	for ( int iDir = 0; iDir < NUM_TERRAIN_DIRECTIONS; ++iDir )
		for ( int iDamage = 0; iDamage < NUM_TERRAIN_DAMAGE; ++iDamage )
			m_aapspriteview[( iDir + iRotRight ) % NUM_TERRAIN_DIRECTIONS ][ iDamage ] =
				terrainsprite.m_aapspriteview[ iDir ][ iDamage ];

	#ifdef _DEBUG	
	CheckValid();
	#endif
}

//---------------------------------------------------------------------------
// CStructureSprite::CStructureSprite
//---------------------------------------------------------------------------
CStructureSprite::CStructureSprite(
	CSpriteParms const & spriteparms )
	:
		CSprite( spriteparms )
{

	theApp.BaseYield ();

	memset( m_aaaapspriteview, 0, sizeof( m_aaaapspriteview ));

	if ( spriteparms.m_bDummy )
		return;

	int	iIndex;
	int	iStage;
	int	iLayerType;
	int	iDir;
	int	iDamage;

	static int	aiIndex[ NUM_BLDG_DIRECTIONS ][ NUM_BLDG_STAGES ][ 3 ][ NUM_BLDG_DAMAGE ];

	int const * piIndex = GetViewIndices();

	for ( iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( iLayerType = 0; iLayerType < 3; ++iLayerType )
				for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
					aiIndex[ iDir ][ iStage ][ iLayerType ][ iDamage ] = *piIndex++;

	#ifdef _GG
	#ifdef _DEBUG	// Check one-piece not combined with fore or back piece
	for ( iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
				ASSERT( -1 == aiIndex[ iDir ][ iStage ][ 0 ][ iDamage ] ||
				      ( -1 == aiIndex[ iDir ][ iStage ][ 1 ][ iDamage ] &&
				        -1 == aiIndex[ iDir ][ iStage ][ 2 ][ iDamage ] ));
	#endif
	#endif

	for ( iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( iLayerType = 0; iLayerType < 3; ++iLayerType )
				for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
				{
					iIndex = aiIndex[ iDir ][ iStage ][ iLayerType ][ iDamage ];

					ASSERT( -1 <= iIndex && iIndex < GetNumViews() );

					if ( !( -1 <= iIndex && iIndex < GetNumViews() ))
						ThrowError ( ERR_TLP_BAD_DATA );

					if ( -1 != iIndex )
						m_aaaapspriteview[ iDir ][ iLayerType >> 1 ][ iStage ][ iDamage ]
							= GetSpriteView( iIndex );
				}

	//	Fill in missing views
	theApp.BaseYield ();

	for ( iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
			{
				if ( !m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ] )
				{
					if ( 0 < iDamage )
					{
						ASSERT( AfxIsValidAddress( m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage - 1 ], sizeof( CSpriteView )));

						m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ]
							=	m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage - 1 ];

						if ( !m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ] )
							m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ]
								=	m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage - 1 ];
					}
					else if ( 0 < iStage )
					{
						ASSERT( AfxIsValidAddress( m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage - 1 ][ iDamage ], sizeof( CSpriteView )));

						m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ]
							=	m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage - 1 ][ iDamage ];

						if ( !m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ] )
							m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ]
								=	m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage - 1 ][ iDamage ];
					}
					else if ( 0 < iDir )
					{
						ASSERT( AfxIsValidAddress( m_aaaapspriteview[ iDir - 1 ][ FOREGROUND_LAYER ][ iStage ][ iDamage ], sizeof( CSpriteView )));

						m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ]
							=	m_aaaapspriteview[ iDir - 1 ][ FOREGROUND_LAYER ][ iStage ][ iDamage ];

						if ( !m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ] )
							m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ]
								=	m_aaaapspriteview[ iDir - 1 ][ BACKGROUND_LAYER ][ iStage ][ iDamage ];
					}
					else
					{
						ASSERT( 0 );
						ThrowError ( ERR_TLP_BAD_DATA );
					}
				}

				// If there are damage overlays, grab the bases from the no-damage view

				else
				{
					if ( 0 <  m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ]->OverlayCount() )
					{
						ASSERT( 0 < iDamage );

						if ( !( 0 < iDamage ))
							ThrowError ( ERR_TLP_BAD_DATA );

						m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ]->
							SetOverlayBase( m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ 0 ] );

						if ( IsTwoPiece( iDir, iStage, iDamage ))
							m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ]->
								SetOverlayBase( m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ 0 ] );
					}

					// Copy animations from base to damage level 1

					if ( 1 == iDamage )
					// if ( 0 < iDamage && iDamage < NUM_BLDG_DAMAGE - 1 )
					{
						m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ iDamage ]->
							SetAnimBase( m_aaaapspriteview[ iDir ][ FOREGROUND_LAYER ][ iStage ][ 0 ] );

						if ( IsTwoPiece( iDir, iStage, iDamage ) && 
						     IsTwoPiece( iDir, iStage, 0 )) 
							m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ iDamage ]->
								SetAnimBase( m_aaaapspriteview[ iDir ][ BACKGROUND_LAYER ][ iStage ][ 0 ] );
					}
				}
			}

	theApp.BaseYield ();

	PostConstruct();

	for ( iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
				if ( IsTwoPiece( iDir, iStage, iDamage ))
					for ( int iZoom = theApp.GetZoomData()->GetFirstZoom(); iZoom < NUM_ZOOM_LEVELS; ++iZoom )
					{
						ASSERT( AfxIsValidAddress( m_aaaapspriteview[iDir][0][iStage][iDamage], sizeof( CSpriteView )));
						ASSERT( AfxIsValidAddress( m_aaaapspriteview[iDir][1][iStage][iDamage], sizeof( CSpriteView )));

						CRect	rect = m_aaaapspriteview[iDir][0][iStage][iDamage]->Rect( iZoom ) |
										 m_aaaapspriteview[iDir][1][iStage][iDamage]->Rect( iZoom );

						m_aaaapspriteview[iDir][FOREGROUND_LAYER][iStage][iDamage]->Rect( iZoom, rect );
						m_aaaapspriteview[iDir][BACKGROUND_LAYER][iStage][iDamage]->Rect( iZoom, rect );
					}

	theApp.BaseYield ();

	#ifdef _DEBUG
	CheckValid();
	#endif
}
	
//---------------------------------------------------------------------------
// CStructureSprite::CStructureSprite
//---------------------------------------------------------------------------
CStructureSprite::CStructureSprite(
	CStructureSprite const & structuresprite,
	int	 					    iID,
	int	 					    iIndex,
	int 						    iRotRight )
	:
		CSprite( structuresprite )
{
	ASSERT( 0 <= iRotRight && iRotRight < NUM_BLDG_DIRECTIONS );

	theApp.BaseYield ();

	SetID   ( iID    );
	SetIndex( iIndex );

	for ( int iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
   	for ( int iLayer = 0; iLayer < NUM_BLDG_LAYERS; ++iLayer )
   		for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
	   		for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
		      	m_aaaapspriteview[( iDir + iRotRight ) % NUM_BLDG_DIRECTIONS ][ iLayer ][ iStage ][ iDamage ] =
				      structuresprite.m_aaaapspriteview[ iDir ][ iLayer ][ iStage ][ iDamage ];

	#ifdef _DEBUG	
	CheckValid();
	#endif
}

//---------------------------------------------------------------------------
// CEffectSprite::CEffectSprite
//---------------------------------------------------------------------------
CEffectSprite::CEffectSprite(
	CSpriteParms const & spriteparms )
	:
		CSprite( spriteparms )
{
	theApp.BaseYield ();

	memset( m_apspriteview, 0, sizeof( m_apspriteview ));

	if ( spriteparms.m_bDummy )
		return;

	int const * piIndex = GetViewIndices();

	for ( int iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( int iLayerType = 0; iLayerType < 3; ++iLayerType )
				for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
				{
					int iIndex = *piIndex++;

					ASSERT( -1 <= iIndex && iIndex < GetNumViews() );

					if ( !( -1 <= iIndex && iIndex < GetNumViews() ))
						ThrowError ( ERR_TLP_BAD_DATA );

					if ( -1 != iIndex	)
						m_apspriteview[ iLayerType >> 1 ] = GetSpriteView( iIndex );
				}

	#ifdef _DEBUG
	m_apspriteview[0]->CheckValid();
	#endif

	PostConstruct();

	if ( IsTwoPiece() )
	{
		ASSERT( AfxIsValidAddress( m_apspriteview[0], sizeof( CSpriteView )));
		ASSERT( AfxIsValidAddress( m_apspriteview[1], sizeof( CSpriteView )));

		for ( int i = theApp.GetZoomData()->GetFirstZoom(); i < NUM_ZOOM_LEVELS; ++i )
		{
			CRect	rect = m_apspriteview[ FOREGROUND_LAYER ]->Rect( i ) |
							 m_apspriteview[ BACKGROUND_LAYER ]->Rect( i );

			m_apspriteview[ FOREGROUND_LAYER ]->Rect( i, rect );
			m_apspriteview[ BACKGROUND_LAYER ]->Rect( i, rect );
		}
	}

	#ifdef _DEBUG
	CheckValid();
	#endif
}
	
//---------------------------------------------------------------------------
// CVehicleSprite::CVehicleSprite
//---------------------------------------------------------------------------
CVehicleSprite::CVehicleSprite(
	CSpriteParms const & spriteparms )
	:
		CSprite( spriteparms )
{
	theApp.BaseYield ();

	memset( m_aaapspriteview, 0, sizeof( m_aaapspriteview ));

	if ( spriteparms.m_bDummy )
		return;

	int	iIndex;
	int	iDir;
	int	iTilt;
	int	iDamage;

	int const * piIndex = GetViewIndices();

	for ( iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
		for ( iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
			for ( iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
			{
				iIndex = *piIndex++;

				ASSERT( -1 <= iIndex && iIndex < GetNumViews() );

				if ( !( -1 <= iIndex && iIndex < GetNumViews() ))
					ThrowError ( ERR_TLP_BAD_DATA );

				if ( -1 != iIndex )
					m_aaapspriteview[ iDir ][ iTilt ][ iDamage ] = GetSpriteView( iIndex );
			}

	for ( iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
		for ( iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
			for ( iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
			{
				if ( !m_aaapspriteview[ iDir ][ iTilt ][ iDamage ] )
				{
					if ( 0 < iDamage )
					{
						ASSERT( AfxIsValidAddress( m_aaapspriteview[ iDir ][ iTilt ][ iDamage - 1 ], sizeof( CSpriteView )));

						m_aaapspriteview[ iDir ][ iTilt ][ iDamage ]
							=	m_aaapspriteview[ iDir ][ iTilt ][ iDamage - 1 ];
					}
					else if ( 0 < iTilt )
					{
						ASSERT( AfxIsValidAddress( m_aaapspriteview[ iDir ][ iTilt - 1 ][ iDamage ], sizeof( CSpriteView )));

						m_aaapspriteview[ iDir ][ iTilt ][ iDamage ]
							=	m_aaapspriteview[ iDir ][ iTilt - 1 ][ iDamage ];
					}
					else if ( 0 < iDir )
					{
						ASSERT( AfxIsValidAddress( m_aaapspriteview[ iDir - 1 ][ iTilt ][ iDamage ], sizeof( CSpriteView )));

						m_aaapspriteview[ iDir ][ iTilt ][ iDamage ]
							=	m_aaapspriteview[ iDir - 1 ][ iTilt ][ iDamage ];
					}
					else
					{
						ASSERT( 0 );
						ThrowError ( ERR_TLP_BAD_DATA );
					}
				}

				// If there are damage overlays, grab the bases from the no-damage view
				else
				{
					if ( 0 < m_aaapspriteview[ iDir ][ iTilt ][ iDamage ]->OverlayCount() )
					{
						ASSERT( 0 < iDamage );

						if ( !( 0 < iDamage ))
							ThrowError( ERR_TLP_BAD_DATA );

						m_aaapspriteview[ iDir ][ iTilt ][ iDamage ]->
							SetOverlayBase( m_aaapspriteview[ iDir ][ iTilt ][ 0 ] );
					}

					// Copy animations from the base

					if ( 0 < iDamage )
						m_aaapspriteview[ iDir ][ iTilt ][ iDamage ]->
							SetAnimBase( m_aaapspriteview[ iDir ][ iTilt ][ 0 ] );
				}
			}

	theApp.BaseYield ();

	PostConstruct();

	theApp.BaseYield ();

	#ifdef _DEBUG
	CheckValid();
	#endif
}
	
//---------------------------------------------------------------------------
// CSpriteCollection::ReadThreadFunc
//---------------------------------------------------------------------------
UINT
CSpriteCollection::ReadThreadFunc(
	void * pvParam )
{
	CThreadParms * pparms 	  = ( CThreadParms * ) pvParam;
	BOOL				bTerminate = FALSE;

	try
	{
		for ( int i = 0; !bTerminate && i < pparms->m_nSprites; ++i )
		{
			Ptr< CSpriteParms > ptrspriteparms = new CSpriteParms( pparms->m_pmmio,
																					 pparms->m_uTime,
																					 pparms->m_iTypeOverride,
																					 pparms->m_iBitsPerPixel );
			g_cs.Lock();

			try
			{
				bTerminate = g_bTerminateReadThread;

				if ( bTerminate )
					ptrspriteparms = NULL;
				else
					g_listptrspriteparms.AddTail( ptrspriteparms );
			}
			catch ( ... )
			{
				g_cs.Unlock();

				throw;
			}

			g_cs.Unlock();
		}
	}
	catch( ... )
	{
		g_cs.Lock();

		g_bReadThreadError = TRUE;

		g_cs.Unlock();
	}

	return 0;
}

//---------------------------------------------------------------------------
// CSpriteCollection::Read
//---------------------------------------------------------------------------
BOOL
CSpriteCollection::Read(
	CMmio &	mmio,
	unsigned uTime,
	int 		iPerStart,
	int		iPerRange,
	int		iBitsPerPixel,
	int		iTypeOverride	)	// type override
{
	if ( iPerStart >= 0 )
		theApp.m_pCreateGame->GetDlgStatus()->SetPer( iPerStart );

	theApp.BaseYield ();

	// make sure the pools are set up
	if ( _memPoolCompSprites == NULL )
		_memPoolCompSprites = MemPoolInit ( MEM_POOL_DEFAULT | MEM_POOL_SERIALIZE );
	if ( _memPoolSprites == NULL )
		_memPoolSprites = MemPoolInit ( MEM_POOL_DEFAULT | MEM_POOL_SERIALIZE );

	ASSERT(  8 == iBitsPerPixel ||
			  15 == iBitsPerPixel ||
			  16 == iBitsPerPixel ||
			  24 == iBitsPerPixel ||
			  32 == iBitsPerPixel );

	char	szBPS[ 3 ];

	if ( 8 == iBitsPerPixel )
		sprintf( szBPS, "08" );
	else
		sprintf( szBPS, "24" );

	ASSERT( !strcmp( szBPS, "08" ) ||
			  !strcmp( szBPS, "24" ));

	mmio.DescendList  ( 'S', 'P', szBPS[0], szBPS[1] );
	mmio.DescendChunk ( 'N', 'U', 'M', 'S' );

	m_nSprite = mmio.ReadShort();

	ASSERT( 0 < m_nSprite && m_nSprite < 1000 );

	if ( !( 0 < m_nSprite && m_nSprite < 1000 ))
		ThrowError( ERR_TLP_BAD_DATA );

	if ( !( 0 < m_nSprite && m_nSprite < 1000 ))
		ThrowError( ERR_TLP_BAD_DATA );

	m_pptrsprite = new Ptr< CSprite > [ m_nSprite ];

	mmio.AscendChunk ();

	int	iPerAdd = 0;
	int	iMaxID  = -1;
	
	CThreadParms threadparms( &mmio, uTime, iTypeOverride, iBitsPerPixel, m_nSprite );
	BOOL			 bReadThread = W32s != iWinType;
	HANDLE		 hThread;

	if ( bReadThread )
	{
		ASSERT( g_listptrspriteparms.IsEmpty() );

		g_listptrspriteparms.RemoveAll();	// Shoudn't have to, but...
		g_bTerminateReadThread = FALSE;
		g_bReadThreadError     = FALSE;

		CWinThread *pthread = AfxBeginThread( ReadThreadFunc, & threadparms, THREAD_PRIORITY_HIGHEST );

		// GG: Hope the thread doesn't terminate before we get here.

		hThread = pthread->m_hThread;
	}

	for ( int i = 0; i < m_nSprite; ++i )
	{
		try
		{
			#ifdef _CHEAT
			if (theApp.GetProfileInt ("Cheat", "ShowLoad", 0))
			{
				CString sText = "Sprite: " + IntToCString (i);
				theApp.m_pCreateGame->GetDlgStatus()->SetMsg (sText);
			}
			#endif

			theApp.BaseYield ();

			if (iPerStart > 0)
			{
				int iAdd = (i * iPerRange) / m_nSprite;

				if (iPerAdd < iAdd)
				{
					iPerAdd = iAdd;
					theApp.m_pCreateGame->GetDlgStatus()->SetPer( iPerStart + iPerAdd );
				}
			}

			Ptr< CSpriteParms >	ptrspriteparms;

			if ( ! bReadThread )
				ptrspriteparms = new CSpriteParms( &mmio, uTime, iTypeOverride, iBitsPerPixel );
			else
				for ( ;; )
				{
					g_cs.Lock();

					try
					{
						if ( g_bReadThreadError )
							ThrowError( ERR_TLP_BAD_DATA );

						if ( ! g_listptrspriteparms.IsEmpty() )
							ptrspriteparms = g_listptrspriteparms.RemoveHead();
					}
					catch( ... )
					{
						g_cs.Unlock();

						throw;
					}

					g_cs.Unlock();

					if ( ptrspriteparms.Value() )
						break;

					theApp.BaseYield();

					Sleep( 0 );
				}

			ptrspriteparms->m_pspritecollection = this;

			m_pptrsprite[ i ] = CSprite::VirtualConstruct( * ptrspriteparms );
			iMaxID 				= max( iMaxID, m_pptrsprite[ i ]->GetID() );
		}
		catch ( ... )
		{
			if ( bReadThread )
			{
				g_cs.Lock();

				g_bTerminateReadThread = TRUE;
				g_listptrspriteparms.RemoveAll();

				g_cs.Unlock();

				// Wait for the read thread to terminate itself (it accesses data local to this function)

				WaitForSingleObject( hThread, INFINITE );
			}

			throw;
		}
	}

	ASSERT( 0 <= iMaxID );

	theApp.BaseYield ();

	m_ptrspritecollectioninfo = new CSpriteCollectionInfo( iMaxID );

	int	iIndex  = -1;
	int	iStartIndex  = 0;
	int	iPrevID;

	if ( 0 <= m_nSprite )
		iPrevID = m_pptrsprite[ 0 ]->GetID();

	for ( i = 0; i < m_nSprite; ++i )
	{
		ASSERT( 0 <= m_pptrsprite[ i ]->GetID() );

		int	iID = m_pptrsprite[ i ]->GetID();

		ASSERT( iIndex != -1 || ( iID == iPrevID && iID == m_pptrsprite[ 0 ]->GetID() ));

		if ( iID == iPrevID )
			iIndex++;
		else
		{
			m_ptrspritecollectioninfo->SetInfo( iPrevID, iStartIndex, iIndex + 1 );

			iStartIndex = i;
			iIndex  		= 0;
			iPrevID 		= iID;
		}

		ASSERT( 0 <= iIndex );

		m_pptrsprite[ i ]->SetIndex( iIndex );

		#ifdef _DEBUG
		m_pptrsprite[ i ]->CheckValid();
		#endif
	}

	if ( 0 < i )
		m_ptrspritecollectioninfo->SetInfo( iPrevID, iStartIndex, iIndex + 1 );

	mmio.AscendList();

	ASSERT_VALID( this );

	m_bOpen = TRUE;

#ifdef _CHEAT
	if (theApp.GetProfileInt ("Cheat", "ShowLoad", 0))
		theApp.m_pCreateGame->GetDlgStatus()->SetMsg ("Sprites Loaded");
#endif

	theApp.BaseYield ();

	MemPoolShrink ( _memPoolCompSprites );

	return FALSE;
}

//---------------------------------------------------------------------------
// CSpriteCollection::SetSprite
//---------------------------------------------------------------------------
void
CSpriteCollection::SetSprite(
	Ptr< CSprite > const & ptrsprite,
	int 						  iID,
	int 						  iIndex )
{
	ASSERT_VALID( this );

	int	i = m_ptrspritecollectioninfo->GetIndex( iID, iIndex );

	m_pptrsprite[ i ] = ptrsprite;
}

//---------------------------------------------------------------------------
// CSpriteCollection::Close
//---------------------------------------------------------------------------
void CSpriteCollection::Close()
{
	ASSERT_VALID( this );

	m_bLoaded = 0;

	delete [] m_pptrsprite;

	m_pptrsprite = NULL;
	m_nSprite    = 0;
	m_bOpen      = FALSE;
	m_ptrfile	 = NULL;
}

//--------------------------------------------------------------------------
// CSpriteCollectionInfo::CSpriteCollectionInfo
//--------------------------------------------------------------------------
CSpriteCollectionInfo::CSpriteCollectionInfo(
	int iMaxID )
	:
		m_iMaxID			( iMaxID ),
		m_pspriteidinfo( NULL )
{
	m_pspriteidinfo = new CSpriteIDInfo [ iMaxID + 1 ];

	ASSERT_VALID( this );
}

//--------------------------------------------------------------------------
// CSpriteCollectionInfo::~CSpriteCollectionInfo
//--------------------------------------------------------------------------
CSpriteCollectionInfo::~CSpriteCollectionInfo()
{
	delete [] m_pspriteidinfo;
}

//--------------------------------------------------------------------------
// CSpriteCollectionInfo::SetInfo
//--------------------------------------------------------------------------
void
CSpriteCollectionInfo::SetInfo(
	int	iID,
	int	iIndex,
	int	nCount )
{
	ASSERT_VALID( this );
	ASSERT( iID <= m_iMaxID );
	
	m_pspriteidinfo[ iID ].SetInfo( iIndex, nCount );
}

// Debugging

//--------------------------------------------------------------------------
// CHotSpotKey::CheckValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CHotSpotKey::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	ASSERT( SMOKE_FLAME_HOTSPOT <= m_eType && m_eType < NUM_HOTSPOT_TYPE );
}
#endif

//--------------------------------------------------------------------------
// CViewCoord::CheckValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CViewCoord::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	for ( int i = theApp.GetZoomData()->GetFirstZoom(); i < NUM_ZOOM_LEVELS; ++i )
	{
		ASSERT( -1024 < m_apt[i].x && m_apt[i].x < 1024 );
		ASSERT( -1024 < m_apt[i].y && m_apt[i].y < 1024 );
	}
}
#endif

//--------------------------------------------------------------------------
// CHotSpot::CheckValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CHotSpot::CheckValid() const
{
	CViewCoord::CheckValid();

	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	m_key.CheckValid();
}
#endif

//--------------------------------------------------------------------------
// CSpriteCollectionInfo::AssertValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteCollectionInfo::AssertValid () const
{
	CObject::AssertValid();

	ASSERT( AfxIsValidAddress( m_pspriteidinfo, ( 1 + m_iMaxID ) * sizeof( CSpriteIDInfo )));
}
#endif

//--------------------------------------------------------------------------
// CSpriteCollection::AssertValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteCollection::AssertValid () const
{
	CObject::AssertValid();

	if ( m_bOpen )
	{
		ASSERT_VALID( m_ptrspritecollectioninfo.Value() );

		for ( int i = 0; i < m_nSprite; ++i )
			ASSERT( AfxIsValidAddress( m_pptrsprite[ i ].Value(), sizeof( CSprite )));
	}
}
#endif

//--------------------------------------------------------------------------
// CEffect::AssertValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CEffect::AssertValid() const
{
	CSpriteStore< CEffectSprite >::AssertValid ();

	ASSERT( this == &theEffects );

	// if not init'ed yet, return
	if ( GetCount() == 0 )
		return;

	ASSERT( 0 <= m_nTrees );

	ASSERT( 0 == m_nTrees || AfxIsValidAddress( m_ptrees, m_nTrees * sizeof( CTree * )));

	for ( int i = 0; i < m_nTrees; ++i )
		ASSERT_STRICT( AfxIsValidAddress( m_ptrees + i, sizeof( CTree )));
}
#endif

//--------------------------------------------------------------------------
// CTurrets::AssertValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CTurrets::AssertValid() const
{
	CSpriteStore< CVehicleSprite >::AssertValid ();

	ASSERT( this == &theTurrets );
}
#endif

//--------------------------------------------------------------------------
// CMuzzleFlashes::AssertValid
//--------------------------------------------------------------------------
#ifdef _DEBUG
void CMuzzleFlashes::AssertValid() const
{
	CSpriteStore< CVehicleSprite >::AssertValid ();

	ASSERT( this == &theFlashes );
}
#endif

//-------------------------------------------------------------------------
// CSpriteDIBParms::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteDIBParms::AssertValid() const
{
	CObject::AssertValid();

	ASSERT( 1 <= m_uTime && m_uTime <= 240 );
	ASSERT( 1 <= m_iType && m_iType <= MAX_DIB_TYPES );
	ASSERT( 8 == m_iBitsPerPixel ||
			 15 == m_iBitsPerPixel ||
			 16 == m_iBitsPerPixel ||
			 24 == m_iBitsPerPixel ||
			 32 == m_iBitsPerPixel );
}
#endif

//-------------------------------------------------------------------------
// CBlockInfo::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CBlockInfo::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	ASSERT( 0L <= m_lOffset );
	ASSERT( 0L <= m_lLength );
}
#endif

//-------------------------------------------------------------------------
// CDIBLayoutInfo::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CDIBLayoutInfo::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	int	i;

	for ( i = 0; i < 4; ++i )
		m_blockinfoDecompressed[i].CheckValid();
}
#endif

//-------------------------------------------------------------------------
// CLayoutInfo::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CLayoutInfo::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	m_blockinfoCompressed.CheckValid();

	int	i;

	for ( i = 0; i < 4; ++i )
		ASSERT( 0 <= m_aiDecompressedLength[i] );

	for ( i = 1; i < 4; ++i )
		ASSERT( m_aiDecompressedLength[i] > m_aiDecompressedLength[i - 1] );
}
#endif

//-------------------------------------------------------------------------
// CSpriteDIB::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteDIB::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));
	ASSERT( 1 <= m_iType && m_iType <= MAX_DIB_TYPES );
	ASSERT( m_iBytesPerPixel == ptrthebltformat->GetBytesPerPixel() );
	ASSERT( 0 <= m_uTime && m_uTime < 240 );

	ASSERT( -1280 < m_arect[0].left  && m_arect[0].left < 1280 );
	ASSERT( -1024 < m_arect[0].top   && m_arect[0].top  < 1024 );
	ASSERT( 0 <= m_arect[0].Width()  && m_arect[0].Width()  <= 1280 );
	ASSERT( 0 <= m_arect[0].Height() && m_arect[0].Height() <= 1024 );

	int	i;

	for ( i = 1 + theApp.GetZoomData()->GetFirstZoom(); i < NUM_ZOOM_LEVELS; ++i )
	{
// GG		ASSERT( m_arect[i].Width()  == m_arect[i-1].Width() >> 1 );
// GG		ASSERT( m_arect[i].Height() == m_arect[i-1].Height() >> 1 );
	}

	for ( i = theApp.GetZoomData()->GetFirstZoom(); i < NUM_ZOOM_LEVELS; ++i )
		m_adiblayoutinfo[i].CheckValid();

	ASSERT( 1 <= m_iBytesPerPixel && m_iBytesPerPixel <= 4 );
}
#endif

//-------------------------------------------------------------------------
// CTile::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CTile::AssertValid () const
{
	CObject::AssertValid ();

	ASSERT( SIMPLE_TILE == ( m_byType & 0x01 ) || UNIT_TILE == ( m_byType & 0x01 ));
}
#endif

//-------------------------------------------------------------------------
// CAmbient::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void 
CAmbient::AssertValid() const
{
}
#endif

//-------------------------------------------------------------------------
// CUnitTile::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CUnitTile::AssertValid () const
{
	CTile::AssertValid();

	ASSERT_STRICT( CStructureSprite::BACKGROUND_LAYER == m_eLayer ||
			  CStructureSprite::FOREGROUND_LAYER == m_eLayer );

	ASSERT_STRICT( 0 <= m_bDamage && m_bDamage < NUM_DAMAGE_LEVELS );
	for ( int i = 0; i < CSpriteView::ANIM_COUNT; ++i )

		ASSERT_STRICT_VALID( m_aambient + i );
}
#endif

//-------------------------------------------------------------------------
// CSimpleTile::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CSimpleTile::AssertValid () const
{
	CTile::AssertValid();
}
#endif

//---------------------------------------------------------------------------
// CSpriteView::CheckValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteView::CheckValid () const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	int	i, j;
/*
	for ( i = 0; i < NUM_ZOOM_LEVELS; ++i )
	{
		ASSERT( 0 <= m_arect[i].Width()  && m_arect[i].Width()  < 1024 );
		ASSERT( 0 <= m_arect[i].Height() && m_arect[i].Height() < 1024 );
	}

	for ( i = 1; i < NUM_ZOOM_LEVELS; ++i )
	{
		ASSERT( m_arect[i].Width()  <= m_arect[i-1].Width()  );
		ASSERT( m_arect[i].Height() <= m_arect[i-1].Height() );
	}

	ASSERT( m_arect[NUM_ZOOM_LEVELS - 1].Height() < m_arect[0].Height() );
	ASSERT( m_arect[NUM_ZOOM_LEVELS - 1].Width()  < m_arect[0].Width()  );
  */
	ASSERT( 0 <= m_nHotSpots && m_nHotSpots < 20 );

	for ( i = 0; i < m_nHotSpots; ++i )
		m_photspots[i].CheckValid();

	ASSERT( 0 <= m_nBase    && m_nBase    < 10 );
	ASSERT( 0 <= m_nOverlay && m_nOverlay < 10 );
	ASSERT( 0 <= m_anAnim[ ANIM_BACK_1  ] && m_anAnim[ ANIM_BACK_1  ] <= 26 );
	ASSERT( 0 <= m_anAnim[ ANIM_BACK_2  ] && m_anAnim[ ANIM_BACK_2  ] <= 26 );
	ASSERT( 0 <= m_anAnim[ ANIM_FRONT_1 ] && m_anAnim[ ANIM_FRONT_1 ] <= 26 );
	ASSERT( 0 <= m_anAnim[ ANIM_FRONT_2 ] && m_anAnim[ ANIM_FRONT_2 ] <= 26 );

	for ( i = 0; i < m_nBase; ++i )
		m_pspritedibBase[i].CheckValid();

	for ( i = 0; i < m_nOverlay; ++i )
		m_pspritedibOverlay[i].CheckValid();

	for ( j = 0; j < ANIM_COUNT; ++j )
		for ( i = 0; i < m_anAnim[j]; ++i )
			m_apspritedibAnim[j][i].CheckValid();

	m_anchor.CheckValid();

	ASSERT( 0 <= m_iSuperviewIndex && m_iSuperviewIndex < m_psprite->GetNumSuperviews() );
}
#endif

//---------------------------------------------------------------------------
// CSpriteParms::CheckValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteParms::CheckValid () const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));
	ASSERT( CSprite::TERRAIN <= m_iType && m_iType <= CSprite::VEHICLE );
	ASSERT( 0 <= m_iID && m_iID < 1000 );
	ASSERT( 8 == m_iBitsPerPixel || 15 == m_iBitsPerPixel || 16 == m_iBitsPerPixel || 24 == m_iBitsPerPixel || 32 == m_iBitsPerPixel );
}
#endif

//---------------------------------------------------------------------------
// CSpriteHdr::CheckValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void CSpriteHdr::CheckValid () const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));
	ASSERT( 0 < m_nViews && m_nViews <= NUM_BLDG_DIRECTIONS * NUM_BLDG_STAGES * NUM_BLDG_LAYERS * NUM_DAMAGE_LEVELS );
	ASSERT( 1 == m_nSuperViews || 4 == m_nSuperViews );

	for ( int i = theApp.GetZoomData()->GetFirstZoom(); i < NUM_ZOOM_LEVELS; ++i )
		m_blockinfoZoom[i].CheckValid();
}
#endif

//---------------------------------------------------------------------------
// CSuperviewInfo::CheckValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void CSuperviewInfo::CheckValid () const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	for ( int i = theApp.GetZoomData()->GetFirstZoom(); i < NUM_ZOOM_LEVELS; ++i )
		m_alayoutinfo[i].CheckValid();
}
#endif

//-------------------------------------------------------------------------
// CSprite::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CSprite::CheckValid() const
{
	ASSERT( AfxIsValidAddress( this, sizeof( *this )));
	ASSERT( 0 <= m_iID    && m_iID    < 1000 );
	ASSERT( 0 <= m_iIndex && m_iIndex < 1000 );
	ASSERT( TERRAIN <= m_iType && m_iType <= VEHICLE );
	ASSERT( 1 <= m_iBytesPerPixel && m_iBytesPerPixel <= 4 );
	ASSERT( m_iBytesPerPixel == ( m_iBitsPerPixel + 7 ) >> 3 );
	ASSERT( m_iBitsPerPixel == ptrthebltformat->GetBitsPerPixel() );

	ASSERT(  8 == m_iBitsPerPixel ||
				15 == m_iBitsPerPixel ||
				16 == m_iBitsPerPixel ||
				24 == m_iBitsPerPixel ||
				32 == m_iBitsPerPixel );

	if ( !m_ptrspritehdr.Value() )
		return;

	int	i;

	for ( i = 0; i < sizeof( m_anHotSpots ) / sizeof( m_anHotSpots[ 0 ] ); ++i )
		ASSERT( 0 <= m_anHotSpots[ i ] && m_anHotSpots[ i ] < 50 );

	ASSERT( AfxIsValidAddress( m_piViewOffsets, sizeof( int ) * GetNumViews() ));
}
#endif

//-------------------------------------------------------------------------
// CTerrainSprite::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CTerrainSprite::CheckValid() const
{
	CSprite::CheckValid();

	ASSERT( AfxIsValidAddress( this, sizeof( *this )));
}
#endif

//-------------------------------------------------------------------------
// CEffectSprite::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CEffectSprite::CheckValid() const
{
	CSprite::CheckValid();

	ASSERT( AfxIsValidAddress( this, sizeof( *this )));
}
#endif

//-------------------------------------------------------------------------
// CStructureSprite::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CStructureSprite::CheckValid() const
{
	CSprite::CheckValid();

	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	for ( int iDir = 0; iDir < NUM_BLDG_DIRECTIONS; ++iDir )
		for ( int iStage = 0; iStage < NUM_BLDG_STAGES; ++iStage )
			for ( int iDamage = 0; iDamage < NUM_BLDG_DAMAGE; ++iDamage )
			{
				int	iLayers = 1 + IsTwoPiece( iDir, iStage, iDamage );

				for ( int iLayer = 0; iLayer < iLayers; ++iLayer )
					ASSERT_STRICT( AfxIsValidAddress(
							m_aaaapspriteview[ iDir ][ iLayer ][ iStage ][ iDamage ].Value(),
							sizeof( CSpriteView )));
			}
}
#endif

//-------------------------------------------------------------------------
// CVehicleSprite::CheckValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void CVehicleSprite::CheckValid() const
{
	CSprite::CheckValid();

	ASSERT( AfxIsValidAddress( this, sizeof( *this )));

	for ( int iDir = 0; iDir < NUM_VEHICLE_DIRECTIONS; ++iDir )
		for ( int iTilt = 0; iTilt < NUM_VEHICLE_TILTS; ++iTilt )
			for ( int iDamage = 0; iDamage < NUM_VEHICLE_DAMAGE; ++iDamage )
				ASSERT_STRICT( AfxIsValidAddress(
							m_aaapspriteview[ iDir ][ iTilt ][ iDamage ].Value(),
							sizeof( CSpriteView )));
}
#endif
